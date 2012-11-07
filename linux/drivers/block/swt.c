
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/swt.h>
#include <linux/string.h>

extern struct nbd_device;
extern static int sock_xmit(struct nbd_device *nbd, int send, void *buf, int size, int msg_flags);
void seek(char * host, char * tail);


int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	int res = 0;

	if(req == NULL || nbd->session == NULL)
		res = identify(nbd->server.host, nbd->server.port, nbd->usr.user, nbd->user);
	else
	{
		//Rest of operations
	}

	return res;
}

struct request *swt_read_stat(struct nbd_device *nbd)
{
	struct request result;
	char reply[4096];
	char garbage[1024];
	char token[512];
	char url[1024];

	sock_xmit(nbd, 0, reply, sizeof(reply), MSG_WAITALL);

	sscanf(reply, swt_ans_format[0], url, token);

	if(url != NULL && token != NULL)
	{
		nbd->session.url = url;
		nbd->session.token = token;
	}else
	{
		//Tiro errado, intentamos con otra operación :)
		
	}

	return (struct request *)&result;
}

int identify(char * host, int * port, char * user, char * key)
{
	char request[1024];

	sprintf(request, swt_ops_format[0], host, user, key);

	return sock_xmit(nbd, 1, request, strlen(request), 0);
}

int listContainers(token, url)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[1], tail, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int createContainers(token, url, name)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[2], tail, name, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int deleteContainers(token, url, name)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[3], tail, name, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int listObjects(token, url, container)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[4], tail, container, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int retrieveObjects(token, url, container, object)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[5], tail, container, object, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int createObjects(token, url, container, object)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[6], tail, container, object, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

int deleteObjects(token, url, container, object)
{
	char request[1024];
	char * host, tail = url;

	seek(host,tail);	

	sprintf(request, swt_ops_format[7], tail, container, object, host, token);  

	return sock_xmit(nbd,1,request,strlen(request),0);
}

void seek(char * host, char * tail)
{
	strsep(&tail, '/');
	strsep(&tail, '/');
	host = strsep(&tail, '/');
}
