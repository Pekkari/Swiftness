
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/nbd.h>
#include <linux/swt.h>
#include <linux/string.h>
#include <linux/socket.h>

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
extern static int sock_xmit(struct nbd_device *nbd, int send, void *buf, int size, \
				int msg_flags);

int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	char request[4096];

       	if(req == NULL || nbd->sess.token == NULL || nbd->sess.url == NULL)
		translate_request(request, SWT_AUTH, nbd->srv.host, nbd->srv.port, \
							nbd->usr.user, nbd->usr.key);	
	else
	{
		//Rest of operations
		translate_request(request, SWT_CONTAINER_LIST, nbd->sess.token, nbd->sess.url);
		
		//translate_request(request, SWT_CONTAINER_CREATE, nbd->sess.token, nbd->sess.url, container);
		//translate_request(request, SWT_CONTAINER_DELETE, nbd->sess.token, nbd->sess.url, container);
		//translate_request(request, SWT_OBJECT_LIST, nbd->sess.token, nbd->sess.url, container);
		//translate_request(request, SWT_OBJECT_RETRIEVE, nbd->sess.token, nbd->sess.url, container, object);
		//translate_request(request, SWT_OBJECT_CREATE, nbd->sess.token, nbd->sess.url, container, object, data, size);
		//translate_request(request, SWT_OBJECT_DELETE, nbd->sess.token, nbd->sess.url, container,object);
	}

	return sock_xmit(nbd, 1, request, strlen(request), 0);
}

struct request *swt_read_stat(struct nbd_device *nbd)
{
	//struct request result;
	char * reply = NULL, * token = NULL, * url = NULL;// * name;
	//void * data;
	//int size = 0;

	sock_xmit(nbd, 0, reply, sizeof(reply), MSG_WAITALL);
	
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
		return NULL;
	}*/
	

	// Aquí debe devolver la request que generó esta reply
	return NULL;
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

				struct container * cont;
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

