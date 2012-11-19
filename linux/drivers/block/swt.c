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
#include <linux/list.h>
#include <linux/kfifo.h>

static LIST_HEAD(unprocessed);
struct container * head;

void translate_request(char * request, int op, ...);
int translate_reply(char * reply, int op, ...);
struct file *reverse_mapping(struct page * pg);
struct swt_op *peek_req_list(struct nbd_device *nbd, struct swt_op *head, \
				struct swt_reply *reply_op);

int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	int result, flags;
	unsigned long size = blk_rq_bytes(req);
	char rest[2024];
	struct swt_op * op_desc = kmalloc(sizeof(struct swt_op),GFP_KERNEL);

/*	dprintk(DBG_TX, "%s: request %p: sending control (%s@%llu,%uB)\n",
			nbd->disk->disk_name, req,
			nbdcmd_to_ascii(nbd_cmd(req)),
			(unsigned long long)blk_rq_pos(req) << 9,
			blk_rq_bytes(req));*/

       	if(req == NULL || nbd->sess.token == NULL || nbd->sess.url == NULL)
	{

		op_desc->op = SWT_AUTH;
		translate_request(rest, SWT_AUTH, nbd->srv.host, nbd->srv.port, \
					nbd->usr.user, nbd->usr.key);

	}
	else
	{
		struct file * filp = reverse_mapping(req->bio->bi_io_vec->bv_page);
		struct inode * inode = filp->f_path.dentry->d_inode; 
		//Rest of operations
		if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISDIR(inode->i_mode))
		{

			op_desc->op = SWT_CONTAINER_LIST;
			translate_request(rest, SWT_CONTAINER_LIST, nbd->sess.token, \
				nbd->sess.url);

		}else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISDIR(inode->i_mode))
		{

			op_desc->op = SWT_CONTAINER_CREATE;
			op_desc->container = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_CONTAINER_CREATE, nbd->sess.token, \
				nbd->sess.url, op_desc->container);

		}else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && IS_DEADDIR(inode))
		{

			op_desc->op = SWT_CONTAINER_DELETE;
			op_desc->container = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_CONTAINER_DELETE, nbd->sess.token, \
				nbd->sess.url, op_desc->container);

		}else if ( filp->f_path.mnt->mnt_root == filp->f_path.dentry->d_parent \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISDIR(inode->i_mode))
		{

			op_desc->op = SWT_OBJECT_LIST;
			op_desc->container = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_OBJECT_LIST, nbd->sess.token,\
				nbd->sess.url, op_desc->container);
				
		}else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& !(req->bio->bi_rw & REQ_WRITE) && S_ISREG(inode->i_mode))
		{
			
			op_desc->op = SWT_OBJECT_RETRIEVE;
			op_desc->container = filp->f_path.dentry->d_parent->d_name.name;
			op_desc->object = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_OBJECT_RETRIEVE, nbd->sess.token, \
				nbd->sess.url, op_desc->container, op_desc->object);

		}else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& req->bio->bi_rw & REQ_WRITE && S_ISREG(inode->i_mode))
		{
			
			op_desc->op = SWT_OBJECT_CREATE;
			op_desc->container = filp->f_path.dentry->d_parent->d_name.name;
			op_desc->object = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_OBJECT_CREATE, nbd->sess.token, \
				nbd->sess.url, op_desc->container, op_desc->object, \
				NULL, size);

		}else if ( filp->f_path.mnt->mnt_root != filp->f_path.dentry->d_parent \
				&& S_ISREG(inode->i_mode) && inode->i_nlink == 0)
		{
			
			op_desc->op = SWT_OBJECT_DELETE;
			op_desc->container = filp->f_path.dentry->d_parent->d_name.name;
			op_desc->object = filp->f_path.dentry->d_name.name;
			translate_request(rest, SWT_OBJECT_DELETE, nbd->sess.token, \
				nbd->sess.url, op_desc->container, op_desc->object);

		}
	}
		
	result = sock_xmit(nbd, 1, &rest, strlen(rest),
			(nbd_cmd(req) == NBD_CMD_WRITE) ? MSG_MORE : 0);
	if (result <= 0) 
	{
		dev_err(disk_to_dev(nbd->disk),
			"Send control failed (result %d)\n", result);
		goto error_out;
	}
	else
	{
		op_desc->req = req;
		if(kfifo_in(nbd->pending, op_desc, sizeof(struct swt_op)) != \
			sizeof(struct swt_op))
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
	int res;
	struct request *req = NULL;
	struct swt_op * head = NULL, * op = NULL;
	char * reply = NULL, * token = NULL, * url = NULL, * container = NULL, * object = NULL;
	struct swt_reply * reply_op = kmalloc(sizeof(struct swt_reply), GFP_KERNEL);
	void * data = NULL;
	int size = 0;

	// Inicializamos la lista
	INIT_LIST_HEAD(&reply_op->unprocessed);

	res = sock_xmit(nbd, 0, &reply, sizeof(reply), MSG_WAITALL);
	if (res <= 0) {
		dev_err(disk_to_dev(nbd->disk),
			"Receive control failed (result %d)\n", res);
		goto harderror;
	}

	if( (res = (2 == sscanf(reply, swt_ans_format[SWT_AUTH], token, url))) )
	{
		// Vaya! Nos identificamos en la nube :)
		nbd->sess.url = url;
		nbd->sess.token = token;
		reply_op->op = SWT_AUTH;

	}else if ( (res = (2 == sscanf(reply, swt_ans_format[SWT_CONTAINER_LIST], \
					container, size))) ) 
	{
		// Tiro errado, listado de objetos?
		reply_op->op = SWT_CONTAINER_LIST;
		op = peek_req_list(nbd, head, reply_op);
		if (req)
			translate_reply(reply, SWT_CONTAINER_LIST);

	}else if ( (res = (!strncmp(reply, swt_ans_format[SWT_CONTAINER_CREATE],\
					strlen(swt_ans_format[SWT_CONTAINER_CREATE])))) )
	{
		// Intentamos con la creación de contenedores :)
		reply_op->op = SWT_CONTAINER_CREATE;
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_CONTAINER_CREATE, op->container);
		
	}else if ( (res = (!strncmp(reply, swt_ans_format[SWT_CONTAINER_DELETE], \
				strlen(swt_ans_format[SWT_CONTAINER_DELETE])))) )
	{
		// Borrado de contenedores?
		reply_op->op = SWT_CONTAINER_DELETE;		
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_CONTAINER_DELETE, op->container);

	}else if ( (res = (2 == sscanf(reply, swt_ans_format[SWT_OBJECT_LIST], \
					object, size))) ) 
	{
		// Grrrr! Listado de objetos?
		reply_op->op = SWT_OBJECT_LIST;
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_OBJECT_LIST, container);

	}else if ( (res = (2 == sscanf(reply, swt_ans_format[SWT_OBJECT_RETRIEVE], \
					object, size))) ) 
	{
		// Ya se! Recuperación de objetos!
		reply_op->op = SWT_OBJECT_RETRIEVE;
		reply_op->size = size;
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_OBJECT_RETRIEVE, op->container,\
					op->object, data, size);

	}else if ( (res = (1 == sscanf(reply, swt_ans_format[SWT_OBJECT_CREATE], \
						size))) ) 
	{
		// No estarás creando un objeto ¿no?
		reply_op->op = SWT_OBJECT_CREATE;
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_OBJECT_CREATE, op->container, \
				op->object, size);

	}else if ( (res = (!strncmp(reply, swt_ans_format[SWT_OBJECT_DELETE], \
				strlen(swt_ans_format[SWT_OBJECT_DELETE])))) ) 
	{
		// Dime que es un borrado de objetos... :'(
		reply_op->op = SWT_OBJECT_DELETE;
		op = peek_req_list(nbd, head, reply_op);
		if (op)
			translate_reply(reply, SWT_OBJECT_DELETE, op->container, \
				op->object);

	}else
	{
		// Wrong way!
		res = PTR_ERR(req);
		if (res != -ENOENT)
			goto harderror;

		dev_err(disk_to_dev(nbd->disk), "Unexpected reply (%p)\n",
			reply);
		res = -EBADR;
		goto harderror;
	}

	if(op)
		req = op->req;

	// Controlar que se llegue aquí con la reply semiprocesada
/*	dprintk(DBG_RX, "%s: request %p: got reply\n",
			nbd->disk->disk_name, req);*/
	if (req && nbd_cmd(req) == NBD_CMD_READ) {
		struct req_iterator iter;
		struct bio_vec *bvec;

		rq_for_each_segment(bvec, req, iter) {
			res = sock_recv_bvec(nbd, bvec);
			if (res <= 0) {
				dev_err(disk_to_dev(nbd->disk), \
						"Receive data failed (result %d)\n",
					res);
				req->errors++;
				return req;
			}
			//dprintk(DBG_RX, "%s: request %p: got %d bytes data\n",
				//nbd->disk->disk_name, req, bvec->bv_len);
		}
	}
	return req;
harderror:
	kfree(reply);
	nbd->harderror = res;
	return NULL;
}
//	list_entry(reply_op->unprocessed.prev, struct swt_reply, unprocessed);
//	if(kfifo_out_peek(nbd->pending, head, sizeof(struct swt_op)) != sizeof(struct swt_op)) goto harderror;

struct swt_op *peek_req_list(struct nbd_device *nbd, struct swt_op *head, \
				struct swt_reply *reply_op)
{
	struct swt_op * op = NULL;

	if (!kfifo_is_empty(nbd->pending) && kfifo_out_peek(nbd->pending, head, \
					sizeof(struct swt_op)) == sizeof(struct swt_op))
	{

		if(head != NULL && head->op == reply_op->op)
		{
				
			// Lo sacamos de la cola y nos cargamos la reply
			if(kfifo_out(nbd->pending, head, sizeof(struct swt_op)))
				return NULL;

			op = head;

		}
		else
		{
			// Metemos el elemento en la pila
			list_add_tail(&reply_op->unprocessed,&unprocessed);
			return NULL;
		}
	}

	return op;
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
	char * h = host, * t = tail;
	strsep(&t, "/");
	strsep(&t, "/");
	h = strsep(&t, "/");
	strcpy(host,h);
	strcpy(tail,t);
}

void identify(char * request, char * host, int port, char * user, char * key)
{
	sprintf(request, swt_ops_format[SWT_AUTH], host, user, key);
}

void listContainers(char * request, char * token, char * url)
{
	char host[256], * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_LIST], tail, host, token);  
}

void createContainer(char * request, char * token, char * url, char * name)
{
	char host[256], * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_CREATE], tail, name, host, token);  
}

void deleteContainer(char * request, char * token, char * url, char * name)
{
	char host[256], * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_DELETE], tail, name, host, token);  
}

void listObjects(char * request, char * token, char * url, char * container)
{
	char host[256], * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_LIST], tail, container, host, token);  
}

void retrieveObject(char * request, char * token, char * url, char * container, char * object)
{
	char host[256], * tail = url;

	splitUrl(host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_RETRIEVE], tail, container, object, host, token);  
}

void createObject(char * request, char * token, char * url, char * container, char * object, void * data, int size)
{
	char host[256], * tail = url;

	splitUrl(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_CREATE], tail, container, object, host, token, size, data); 
}

void deleteObject(char * request, char * token, char * url, char * container, char * object)
{
	char host[256], * tail = url;

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
	struct container * cont = NULL, * prev_cont = NULL;
	struct object * obj = NULL, * prev_obj = NULL;
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

			// Limpiamos la lista si está llena
			if (head != NULL)
				freeContainers();	

			while((res = (2 == sscanf(reply, \
					swt_ans_format[SWT_CONTAINER_LIST], \
					container, size)))) 
			{
				cont = kmalloc(sizeof(struct container), \
							GFP_KERNEL);
				cont->name = container;
				cont->size = size;
				prev_cont->next = cont;
				prev_cont = cont;

				updateReply(reply);

			}
			break;

		case SWT_CONTAINER_CREATE:

			// Generamos el contenedor y lo metemos en la lista de
			// de contenedores

			container = va_arg(args, char *);
			prev_cont = seekContainer(container);
			cont = kmalloc(sizeof(struct container), GFP_KERNEL);
			cont->name = container;
			cont->size = size;
			cont->next = prev_cont->next;
			prev_cont->next = cont;
			break;

		case SWT_CONTAINER_DELETE:

			// Buscamos el contenedor y lo eliminamos de la lista

			container = va_arg(args, char *);
			prev_cont = seekContainer(container);
			cont = prev_cont->next;
			prev_cont->next = cont->next;
			kfree(cont);
			break;

		case SWT_OBJECT_LIST:
		
			// Seguimos procesando la lista

			container = va_arg(args, char *);

			if ((res=(1 == sscanf(reply, "%*s\n<container name =%s>%*s", \
				container))))
				cont = matchContainer(container);

			if ( cont != NULL)
			{

				// Limpiamos la lista si está llena
				if (cont->first != NULL)
					freeObjects(cont);	

				while((res = (2 == sscanf(reply, \
					swt_ans_format[SWT_OBJECT_LIST], \
					object, size)))) 
				{
					obj = kmalloc(sizeof(struct object), \
								GFP_KERNEL);
					obj->name = container;
					obj->size = size;
					prev_obj->next = obj;
					prev_obj = obj;

					updateReply(reply);

				}

			}
			else res = 0;
			break;

		case SWT_OBJECT_RETRIEVE:

			data = va_arg(args, void *);
			size = va_arg(args, int);
			res = (2 == sscanf(reply, swt_ans_format[SWT_OBJECT_LIST], \
					size, data));
			break;

		case SWT_OBJECT_CREATE:

			container = va_arg(args, char *);
			object = va_arg(args, char *);
			size = va_arg(args, int);
			// Inserta el objeto en la lista

			cont = matchContainer(container);
			
			if ( cont != NULL )
			{

				prev_obj = seekObject(cont, object);
				obj = kmalloc(sizeof(struct object), GFP_KERNEL);
				obj->name = object;
				obj->size = size;
				obj->next = prev_obj->next;
				prev_obj->next = obj;

			}
			break;

		case SWT_OBJECT_DELETE:

			// Buscamos el objeto y lo retiramos de la lista de objetos

			container = va_arg(args, char *);
			object = va_arg(args, char *);
			cont = matchContainer(container);

			if ( cont != NULL )
			{
				prev_obj = seekObject(cont, object);
				obj = prev_obj->next;
				prev_obj->next = obj->next;
				kfree(obj);
			}
			break;

	}
	va_end(args);
	return res;
}

