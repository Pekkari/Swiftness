
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/nbd.h>
#include <linux/swt.h>
#include <linux/string.h>
#include <linux/socket.h>

void translate_request(char * request, int op, ...);

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
	char * reply = NULL, * token = NULL, * url = NULL;

	sock_xmit(nbd, 0, reply, sizeof(reply), MSG_WAITALL);

	
	if( 2 == sscanf(reply, swt_ans_format[SWT_AUTH], url, token) )
	{
		nbd->sess.url = url;
		nbd->sess.token = token;
	}else
	{
		//Tiro errado, intentamos con otra operación :)
		
	}

	// Aquí debe devolver la request que generó esta reply
	return NULL;
}

void seek(char * host, char * tail)
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

	seek(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_LIST], tail, host, token);  
}

void createContainer(char * request, char * token, char * url, char * name)
{
	char * host = url, * tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_CREATE], tail, name, host, token);  
}

void deleteContainer(char * request, char * token, char * url, char * name)
{
	char * host = url, * tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_DELETE], tail, name, host, token);  
}

void listObjects(char * request, char * token, char * url, char * container)
{
	char * host = url, * tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_LIST], tail, container, host, token);  
}

void retrieveObject(char * request, char * token, char * url, char * container, char * object)
{
	char * host = url, * tail = url;

	seek(host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_RETRIEVE], tail, container, object, host, token);  
}

void createObject(char * request, char * token, char * url, char * container, char * object, void * data, int size)
{
	char * host = url, * tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[SWT_OBJECT_CREATE], tail, container, object, host, token, size, data); 
}

void deleteObject(char * request, char * token, char * url, char * container, char * object)
{
	char * host = url, * tail = url;

	seek(host,tail);	

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


