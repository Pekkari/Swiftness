
#ifndef _SWT_H
#define _SWT_H

#include <linux/types.h>

extern struct nbd_device nbd;

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
	"GET /v1.0 HTTP/1.1\nHost: %s\nX-Auth-User: %s\nX-Auth-Key: %s\n",
	"GET /%s?format=xml HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"DELETE /%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s HTTP/1.1\n Host: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\nContent-Length: %i\n%s",
	"DELETE /%s/%s/%s HTTP/1.1\nHost: %s\n X-Auth-Token: %s\n"
};

char * swt_ans_format[] = {
	"HTTP/1.1 204 No Content\n%*s\nX-Storage-Url: %s\nX-Storage-Token: %s\n%*s",
	"%*s<name>%s</name>\n%*s\n<bytes>%d</bytes>\n",
	"HTTP/1.1 201 Created\n",
	"HTTP/1.1 204 No Content\n",
	"%*s<object>\n%*s<name>%s</name>\n%*s<bytes>%d</bytes>\n%*s</object>\n",
	"HTTP/1.1 200 Ok%*s\nContent-Length: %d\n%s",
	"HTTP/1.1 201 Created\n%*s\nContent-Length: %d\n%*s",
	"HTTP/1.1 204 No Content\n%*s"
};

struct swt_auth {
	char * user;
	char * key;
};

struct swt_serv {
	char * host;
	int port;
};

struct swt_sess {
	char * url;
	char * token;
};

#ifdef CONFIG_BLK_DEV_SWT

int swt_send_req(struct nbd_device *nbd, struct request *req);
struct request *swt_read_stat(struct nbd_device *nbd);

#else
int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	return -ENOSYS;
}
struct request *swt_read_stat(struct nbd_device *nbd)
{
	return NULL;
}
#endif
#endif
