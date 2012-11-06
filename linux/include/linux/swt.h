
#ifndef _SWT_H
#define _SWT_H

#include <linux/types.h>

extern struct nbd_device;

enum swt_ops {
	SWT_AUTH,
	SWT_CONTAINER_LIST,
	SWT_CONTAINER_CREATE,
	SWT_CONTAINER_DELETE,
	SWT_OBJECT_LIST,
	SWT_OBJECT_RETRIEVE,
	SWT_OBJECT_CREATE,
	SWT_OBJECT_DELETE,
};

char * swt_ops_format[] = {
	"GET /%s HTTP/1.1\nHost: %s\nX-Auth-User: %s\nX-Auth-Key: %s\n",
	"GET /%s?format=xml HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"DELETE /%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s HTTP/1.1\n Host: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\nContent-Length: %i\n",
	"DELETE /%s/%s/%s HTTP/1.1\nHost: %s\n X-Auth-Token: %s\n"
};

struct swt_auth {
	char user[256];
	char key[256];
};

struct swt_serv {
	char host[256];
	int port;
};

struct swt_sess {
	char url[256];
	char token[256];
};

int swt_send_req(struct nbd_device *nbd, struct request *req);
struct request *swt_read_stat(struct nbd_device *nbd);

#else
int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	return -ENOSYS;
}
struct request *swt_read_stat(struct nbd_device *nbd)
{
	return -ENOSYS;
}
#endif
