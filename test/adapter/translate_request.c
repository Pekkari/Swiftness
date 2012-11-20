#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "swt.h"

void splitUrl(char * url, char * host, char * tail)
{
	char * h = host, * t = tail;
	strcpy(tail, url);
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
	char host[256], tail[256];

	splitUrl(url,host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_LIST], tail, host, token);  
}

void createContainer(char * request, char * token, char * url, char * name)
{
	char host[256], tail[256];

	splitUrl(url, host, tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_CREATE], tail, name, host, token);  
}

void deleteContainer(char * request, char * token, char * url, char * name)
{
	char host[256], tail[256];

	splitUrl(url, host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_DELETE], tail, name, host, token);  
}

void listObjects(char * request, char * token, char * url, char * container)
{
	char host[256], tail[256];

	splitUrl(url,host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_LIST], tail, container, host, token);  
}

void retrieveObject(char * request, char * token, char * url, char * container, char * object)
{
	char host[256], tail[256];

	splitUrl(url,host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_RETRIEVE], tail, container, object, host, token);  
}

void createObject(char * request, char * token, char * url, char * container, char * object, void * data, long size)
{
	char host[256], tail[256];

	splitUrl(url,host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_CREATE], tail, container, object, host, token, size, data); 
}

void deleteObject(char * request, char * token, char * url, char * container, char * object)
{
	char host[256], tail[256];

	splitUrl(url,host,tail);

	sprintf(request, swt_ops_format[SWT_OBJECT_DELETE], tail, container, object, host, token);  
}

void translate_request(char * request, int op, ...)
{
	char * host, * user, * pass, * token, * url, * container, * object;
	void * data;
	int port;
	long size;
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

int main(void)
{
	char host[] = "www.wordpress.com";
	char user[] = "PikkuJose";
	char pass[] = "Betis";
	char token[] = "PararaPaParadudaPaparadudaPaPa";
	char url[] = "http://www.wordpress.com/Auth/v1.0/PikkuJose";
	char container[] = "JarritaDeCerveza";
	char object[] = "ConAceitunas";
	char data[] = "ariofuhsidfhuefheuobcvbdbadfagfkjegaewugfndbaskjdb";
	int port= 63282;
	long size = 35;
	char request[1024];
	
	translate_request(request, SWT_AUTH, host, port, user, pass);
	printf("ID Request:\n%s", request);

	translate_request(request, SWT_CONTAINER_LIST, token, url);
	printf("ListContainers Request:\n%s", request);

	translate_request(request, SWT_CONTAINER_CREATE, token, url, container);
	printf("CreateContainer Request:\n%s", request);

	translate_request(request, SWT_CONTAINER_DELETE, token, url, container);
	printf("DeleteContainer Request:\n%s", request);

	translate_request(request, SWT_OBJECT_LIST, token, url, container);
	printf("ListObjects Request:\n%s", request);

	translate_request(request, SWT_OBJECT_RETRIEVE, token, url, container, object);
	printf("RetrieveObject Request:\n%s", request);

	translate_request(request, SWT_OBJECT_CREATE, token, url, container, object, (void *)data, size);
	printf("CreateObject Request:\n%s", request);

	translate_request(request, SWT_OBJECT_DELETE, token, url, container, object);
	printf("DeleteObject Request:\n%s", request);


	return 0;
}
