/*
 * drivers/block/swt.c - Support for Openstack Swift through the nbd module
 *
 * Copyright 2012, José Ramón Muñoz Pekkarinen <koalinux@gmail.com>
 * Released under the General Public License (GPL).
 *
 */

#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/nbd.h>
#include <linux/swt.h>
#include <linux/string.h>
#include <linux/socket.h>
#include <linux/rmap.h>
#include <linux/mount.h>

struct object
{
	char * name;
	int size;
	struct object * next;
};

struct container
{
	char * name;
	int size;
	struct container * next;
	struct object * first;
};

struct container * head;

void translate_request(char * request, int op, ...);
int translate_reply(char * reply, int op, ...);
struct file *reverse_mapping(struct page * pg);

int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	int result, flags;
	unsigned long size = blk_rq_bytes(req);
	char rest[4096];

	// Way to know where it starts
	//request.from = cpu_to_be64((u64)blk_rq_pos(req) << 9);

/*	dprintk(DBG_TX, "%s: request %p: sending control (%s@%llu,%uB)\n",
			nbd->disk->disk_name, req,
			nbdcmd_to_ascii(nbd_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req));*/

       	if(req == NULL || nbd->sess.token == NULL || nbd->sess.url == NULL)
		translate_request(rest, SWT_AUTH, nbd->srv.host, nbd->srv.port, \
					nbd->usr.user, nbd->usr.key);	
	else
	{
		struct file * filp = reverse_mapping(req->bio->bi_io_vec->bv_page);
		struct inode * inode = filp->f_path.dentry->d_inode; 
		//Rest of operations
		if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISDIR(inode->i_mode))
			translate_request(rest, SWT_CONTAINER_LIST, nbd->sess.token, \
				nbd->sess.url);

		else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISDIR(inode->i_mode))
			translate_request(rest, SWT_CONTAINER_CREATE, nbd->sess.token, \
				nbd->sess.url, filp->f_path.dentry->d_name.name);

		// Remiendo requerido...
		else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISDIR(inode->i_mode))
			translate_request(rest, SWT_CONTAINER_DELETE, nbd->sess.token, \
				nbd->sess.url, filp->f_path.dentry->d_name.name);

		else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISDIR(inode->i_mode))
			translate_request(rest, SWT_OBJECT_LIST, nbd->sess.token,\
				nbd->sess.url, filp->f_path.dentry->d_name.name);
				
		else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISREG(inode->i_mode))
			translate_request(rest, SWT_OBJECT_RETRIEVE, nbd->sess.token, \
				nbd->sess.url, filp->f_path.dentry->d_parent->d_name.name,\
				filp->f_path.dentry->d_name.name);

		else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISREG(inode->i_mode))
		// Remiendo requerido...
			translate_request(rest, SWT_OBJECT_CREATE, nbd->sess.token, \
				nbd->sess.url, filp->f_path.dentry->d_parent->d_name.name,\
				filp->f_path.dentry->d_name.name, data, size);

		// Remiendo requerido...
		else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISREG(inode->i_mode))
			translate_request(rest, SWT_OBJECT_DELETE, nbd->sess.token, \
				nbd->sess.url, filp->f_path.dentry->d_parent->d_name.name,\
				filp->f_path.dentry->d_name.name);
	}
		
	result = sock_xmit(nbd, 1, &rest, strlen(rest),
			(nbd_cmd(req) == NBD_CMD_WRITE) ? MSG_MORE : 0);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}

	if (nbd_cmd(req) == NBD_CMD_WRITE) {
		struct req_iterator iter;
		struct bio_vec *bvec;
		/*
		 * we are really probing at internals to determine
		 * whether to set MSG_MORE or not...
		 */
		rq_for_each_segment(bvec, req, iter) {
			flags = 0;
			if (!rq_iter_last(req, iter))
				flags = MSG_MORE;
			//dprintk(DBG_TX, "%s: request %p: sending %d bytes data\n",
			//		nbd->disk->disk_name, req, bvec->bv_len);
			// Creo que no lo tengo que tocar, pero por siaca
			result = sock_send_bvec(nbd, bvec, flags);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk),
					"Send data failed (result %d)\n",
					result);
				goto error_out;
			}
		}
	}
	return 0;

	error_out:
	return -EIO;
}

struct request *swt_read_stat(struct nbd_device *nbd)
{
	int result;
	struct request *req;
	char * reply = NULL, * token = NULL, * url = NULL;// * name;
	//void * data;
	//int size = 0;

	// Acotar el tamaño de la respuesta.
	result = sock_xmit(nbd, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (result <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Receive control failed (result %d)\n", result);
		goto harderror;
	}

	// Antiguo mecanismo de búsqueda de request no válido
	//req = nbd_find_request(nbd, *(struct request **)reply.handle);

	if( translate_reply(reply, SWT_AUTH, url, token) )
	{
		// Vaya! Nos identificamos en la nube :)
		nbd->sess.url = url;
		nbd->sess.token = token;
	}/*else if ( translate_reply(reply, SWT_CONTAINER_LIST, url, token) ) 
	{
		// Tiro errado, listado de objetos?

	}else if ( translate_reply(reply, SWT_CONTAINER_CREATE) )
	{
		// Intentamos con la creación de contenedores :)
		
	}else if ( translate_reply(reply, SWT_CONTAINER_DELETE) )
	{
		// Borrado de contenedores?
		
	}else if ( translate_reply(reply, SWT_OBJECT_LIST) ) 
	{
		// Grrrr! Recuperación de objetos?

	}else if ( translate_reply(reply, SWT_OBJECT_RETRIEVE, data, size) ) 
	{
		// Ya se! Recuperación de objetos!

	}else if ( translate_reply(reply, SWT_OBJECT_CREATE, size) ) 
	{
		// No estarás creando un objeto ¿no?

	}else if ( translate_reply(reply, SWT_OBJECT_DELETE) ) 
	{
		// Dime que es un borrado de objetos... :'(
	}else
	{
		// Wrong way!
		result = PTR_ERR(req);
		if (result != -ENOENT)
			goto harderror;

		dev_err(disk_to_dev(nbd->disk), "Unexpected reply (%p)\n",
			reply.handle);
		result = -EBADR;
		goto harderror;
	}*/


/*	dprintk(DBG_RX, "%s: request %p: got reply\n",
			nbd->disk->disk_name, req);*/
	if (nbd_cmd(req) == NBD_CMD_READ) {
		struct req_iterator iter;
		struct bio_vec *bvec;

		rq_for_each_segment(bvec, req, iter) {
			result = sock_recv_bvec(nbd, bvec);
			if (result <= 0) {
				dev_err(disk_to_dev(nbd->disk), \
						"Receive data failed (result %d)\n",
					result);
				req->errors++;
				return req;
			}
			//dprintk(DBG_RX, "%s: request %p: got %d bytes data\n",
				//nbd->disk->disk_name, req, bvec->bv_len);
		}
	}
	return req;
harderror:
	nbd->harderror = result;
	return NULL;
}

struct file *reverse_mapping(struct page * pg)
{
	struct address_space *mapping = pg->mapping;
	pgoff_t pgoff = pg->index << (PAGE_CACHE_SHIFT - PAGE_SHIFT);
	struct vm_area_struct *vma;
	struct prio_tree_iter iter;
	struct file * f = NULL;

	mutex_lock(&mapping->i_mmap_mutex);
	vma_prio_tree_foreach(vma, &iter, &mapping->i_mmap, pgoff, pgoff) {
		unsigned long address = page_address_in_vma(pg, vma);
		if (address == -EFAULT)
			continue;
		else
		{
			f = vma->vm_file;
			goto out;
		}
	}
	out:
	mutex_unlock(&mapping->i_mmap_mutex);
	return f;
}

void splitUrl(char * host, char * tail)
{
	strsep(&tail, "/");
	strsep(&tail, "/");
	host = strsep(&tail, "/");
}

void identify(char * request, char * host, int port, char * user, char * key)
{
	sprintf(request, swt_ops_format[SWT_AUTH], host, user, key);
}

void listContainers(char * request, char * token, char * url)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_LIST], tail, host, token);  
}

void createContainer(char * request, char * token, char * url, char * name)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_CREATE], tail, name, host, token);  
}

void deleteContainer(char * request, char * token, char * url, char * name)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_DELETE], tail, name, host, token);  
}

void listObjects(char * request, char * token, char * url, char * container)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_LIST], tail, container, host, token);  
}

void retrieveObject(char * request, char * token, char * url, char * container, char * object)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_RETRIEVE], tail, container, object, host, token);  
}

void createObject(char * request, char * token, char * url, char * container, char * object, void * data, int size)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_CREATE], tail, container, object, host, token, size, data); 
}

void deleteObject(char * request, char * token, char * url, char * container, char * object)
{
	char * host = url, * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_DELETE], tail, container, object, host, token);  
}

void translate_request(char * request, int op, ...)
{
	char * host, * user, * pass, * token, * url, * container, * object;
	void * data;
	int port, size;
	va_list args;
	va_start(args, op);
	
	if (op == SWT_AUTH)
	{
		host = va_arg(args, char *);
		port = va_arg(args, int);
		user = va_arg(args, char *);
		pass = va_arg(args, char *);
		identify(request, host, port, user, pass);
	}
	else
	{
		token = va_arg(args, char *);
		url = va_arg(args, char *);

		switch (op)
		{
			case SWT_CONTAINER_LIST:
				listContainers(request, token, url);
				break;
			case SWT_CONTAINER_CREATE:
				container = va_arg(args, char *);
				createContainer(request, token, url, container);
				break;
			case SWT_CONTAINER_DELETE:
				container = va_arg(args, char *);
				deleteContainer(request, token, url, container);
				break;
			case SWT_OBJECT_LIST:
				container = va_arg(args, char *);
				listObjects(request, token, url, container);
				break;
			case SWT_OBJECT_RETRIEVE:
				container = va_arg(args, char *);
				object = va_arg(args, char *);
				retrieveObject(request, token, url, container, object);
				break;
			case SWT_OBJECT_CREATE:
				container = va_arg(args, char *);
				object = va_arg(args, char *);
				data = va_arg(args, void *);
				size = va_arg(args, int);
				createObject(request, token, url, container, object, data, size);
				break;
			case SWT_OBJECT_DELETE:
				container = va_arg(args, char *);
				object = va_arg(args, char *);
				deleteObject(request, token, url, container, object);
				break;
		}
	}
	va_end(args);
}

void freeContainers(void)
{
	while (head != NULL)
	{
		struct container * nxt = head->next;
		kfree(head);
		head = nxt;
	}
}

void freeObjects(struct container * cont)
{
	while (head != NULL)
	{
		struct object * nxt = cont->first;
		kfree(cont->first);
		cont->first = nxt;
	}
}

void updateReply(char * reply)
{
	do
	{
		strsep(&reply, "<");
	}while (strncmp(reply, "size", strlen("size")));

	strsep(&reply, ">");
}

struct container * seekContainer(char * name)
{
	struct container * cont = head;

	while (0 > strcmp(cont->name, name))
		cont = cont->next;

	return cont;
}

struct container * matchContainer(char * name)
{
	struct container * cont;

	cont = seekContainer(name);

	if (strcmp(cont->name,name))
		cont = NULL;

	return cont;
}

struct object * seekObject(struct container * cont, char * name)
{
	struct object * obj = cont->first;

	while (0 > strcmp(obj->name, name))
		obj = obj->next;

	return obj;
}

struct object * matchobject(struct container * cont, char * name)
{
	struct object * obj;

	obj = seekObject(cont, name);

	if (strcmp(obj->name,name))
		obj = NULL;

	return obj;
}

int translate_reply(char * reply, int op, ...)
{
	char * token = NULL, * url = NULL, * container = NULL, * object = NULL;
	void * data;
	int size = 0, res = 0;
	va_list args;
	va_start(args, op);
	
	switch (op)
	{
		case SWT_AUTH:

			token = va_arg(args, char *);
			url = va_arg(args, char *);
 			res = (2 == sscanf(reply, swt_ans_format[SWT_AUTH], token, url));

		case SWT_CONTAINER_LIST:

			if ((res = (2 == sscanf(reply, swt_ans_format[SWT_CONTAINER_LIST], \
						container, size))))
			{
				// Seguimos procesando la lista
				struct container * cont, * prev;

				// Limpiamos la lista si está llena
				if (head != NULL)
					freeContainers();	

				cont = kmalloc(sizeof(struct container), GFP_KERNEL);
				cont->name = container;
				cont->size = size;
				head = prev = cont;
				
				updateReply(reply);	
				if ((res = (2 == sscanf(reply, \
					swt_ans_format[SWT_CONTAINER_LIST], container, size))))
				{
					do 
					{
						cont = kmalloc(sizeof(struct container), \
									GFP_KERNEL);
						cont->name = container;
						cont->size = size;
						prev->next = cont;
						prev = cont;

						updateReply(reply);

					}while((res = (2 == sscanf(reply, \
							swt_ans_format[SWT_CONTAINER_LIST], \
							container, size))));
				}
			}
			break;

		case SWT_CONTAINER_CREATE:

			if ((res = (!strncmp(reply, swt_ans_format[SWT_CONTAINER_CREATE],\
					strlen(swt_ans_format[SWT_CONTAINER_CREATE])))))
			{
				// Generamos el contenedor y lo metemos en la lista de
				// de contenedores
				struct container * cont, * prev;
				char containerName[256];

				// find container name
				prev = seekContainer(containerName);
				cont = kmalloc(sizeof(struct container), GFP_KERNEL);
				cont->name = containerName;
				cont->size = size;
				cont->next = prev->next;
				prev->next = cont;
			}
			break;

		case SWT_CONTAINER_DELETE:

			if ((res = (!strncmp(reply, swt_ans_format[SWT_CONTAINER_DELETE], \
				strlen(swt_ans_format[SWT_CONTAINER_DELETE])))))
			{
				// Buscamos el contenedor y lo eliminamos de la lista
				struct container * cont, * prev;
				char containerName[256];

				// find container name
				prev = seekContainer(containerName);
				cont = prev->next;
				prev->next = cont->next;
				kfree(cont);
			}
			break;

		case SWT_OBJECT_LIST:
		
			if ((res = (2 == sscanf(reply, swt_ans_format[SWT_OBJECT_LIST], \
					object, size))))
			{
				// Seguimos procesando la lista

				struct container * cont = NULL;
				struct object * obj, * prev;
				char containerName[256];

				if ((res=(1 == sscanf(reply, "%*s\n<container name =%s>%*s", \
					containerName))))
					cont = matchContainer(containerName);

				if ( cont != NULL)
				{

					// Limpiamos la lista si está llena
					if (cont->first != NULL)
						freeObjects(cont);	

					obj = kmalloc(sizeof(struct object), GFP_KERNEL);
					obj->name = object;
					obj->size = size;
					prev = obj;
					
					updateReply(reply);	
					if ((res = (2 == sscanf(reply, \
						swt_ans_format[SWT_OBJECT_LIST], \
						object, size))))
					{
						do 
						{
							obj = kmalloc(sizeof(struct object), \
										GFP_KERNEL);
							obj->name = container;
							obj->size = size;
							prev->next = obj;
							prev = obj;

							updateReply(reply);

					}while((res = (2 == sscanf(reply, \
							swt_ans_format[SWT_OBJECT_LIST], \
							object, size))));
					}

				}
				else res = 0;
			}
			break;

		case SWT_OBJECT_RETRIEVE:

			data = va_arg(args, void *);
			size = va_arg(args, int);
			res = (2 == sscanf(reply, swt_ans_format[SWT_OBJECT_LIST], \
					size, data));
			break;

		case SWT_OBJECT_CREATE:

			size = va_arg(args, int);
			if ((res = (1 == sscanf(reply, swt_ans_format[SWT_OBJECT_CREATE], \
						size))))
			{
				// Inserta el objeto en la lista
				struct container * cont;
				struct object * obj, * prev;
				char containerName[256], objectName[1024];

				// find container name
				cont = matchContainer(containerName);
				
				if ( cont != NULL )
				{
					// find object name
					prev = seekObject(cont, objectName);
					obj = kmalloc(sizeof(struct object), GFP_KERNEL);
					obj->name = objectName;
					obj->size = size;
					obj->next = prev->next;
					prev->next = obj;
				}
			}
			break;

		case SWT_OBJECT_DELETE:
			if ((res = (!strncmp(reply, swt_ans_format[SWT_OBJECT_DELETE], \
				strlen(swt_ans_format[SWT_OBJECT_DELETE])))))
			{
				// Buscamos el objeto y lo retiramos de la lista de objetos
				struct container * cont;
				struct object * obj, * prev;
				char containerName[256],objectName[1024];

				// find container name
				cont = matchContainer(containerName);
				if ( cont != NULL )
				{
					// find object name
					prev = seekObject(cont, objectName);
					obj = prev->next;
					prev->next = obj->next;
					kfree(obj);
				}
			}
			break;

	}
	va_end(args);
	return res;
}

