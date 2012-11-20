#include <stdio.h>
#include <string.h>
#include "swt.h"

void identify(char * request, char * host, int port, char * user, char * key)
{
	sprintf(request, swt_ops_format[SWT_AUTH], host, user, key);
}

int main(void)
{
	char request[200];
	char host[] = "madredediox.com";
	char user[] = "PikkuJose";
	char key[] = "EjemploDeComoNoHacerUnaContraseña";
	int port = 80;

	identify(request, host, port, user, key);
	printf("Request:\n%s", request);
	return 0;
}
