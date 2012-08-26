
#ifndef _SWT_H
#define _SWT_H

enum swt_rest_cmd {
	SWT_GET,
	SWT_PUT,
	SWT_DELETE,
};

enum swt_ops {
	SWT_AUTH,
	SWT_CONTAINER_LIST,
	SWT_CONTAINER_CREATE,
	SWT_CONTAINER_DELETE,
	SWT_OBJECT_LIST,
	SWT_OBJECT_RETRIEVE,
	SWT_OBJECT_CREATE,
	SWT_OBJECT_COPY,
	SWT_OBJECT_DELETE,
};

char * swt_ops_format[] = {
	"GET /%s HTTP/1.1\nHost: %s\nX-Auth-User: %s\nX-Auth-Key: %s\n",
	"GET /%s/<account>?format=%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s/%s[?parm=value] HTTP/1.1\n Host: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"DELETE /%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"GET /%s/%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\n",
	"PUT /%s/%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\nContent-Length: <longitud en bytes>\nX-Object-Meta-PIN: 1234\n\n",
	"PUT /%s/%s/%s/%s HTTP/1.1\nHost: %s\nX-Auth-Token: %s\nX-Copy-From: /%s/%s\nContent-Length: 0 \n",
	"DELETE /%s/%s/%s/%s HTTP/1.1\n	Host: %s\n X-Auth-Token: %s\n"
};

char * api_version[] = {
	"v1.0",
	"Other"
};

struct swt_request {
	enum swt_rest_cmd cmd;
	char * api_vers;
	char * host;
	char * url;
	char * token;
} __attribute__((packed));

struct swt_auth {
	char user[256];
	char key[256];
};

int swt_send_req(struct nbd_device *nbd, struct request *req);

#else
static int swt_send_req(struct nbd_device *nbd, struct request *req)
{
	return -ENOSYS;
}

#endif
