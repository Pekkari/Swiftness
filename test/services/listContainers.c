#include <stdio.h>
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

void listContainers(char * request, char * token, char * url)
{
	char host[256], tail[256];

	splitUrl(url,host,tail);	

	sprintf(request, swt_ops_format[SWT_CONTAINER_LIST], tail, host, token);  
}

int main(void)
{
	char request[200];
	char token[] = "aldkfhvcuihefasclhaskvjlhds239hphsfha9";
	char url[] = "http://www.wordpress.com/Auth/v1.0/PikkuJose";

	listContainers(request, token, url);
	printf("Request:\n%s", request);
	return 0;
}
