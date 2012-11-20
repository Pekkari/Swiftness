#include <stdio.h>
#include <string.h>

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

int main(void)
{
	char theGood[] = "http://example.com/authv1/PikkuJose";
	char theBad[] = "http://example.com//jander.com";
	char theUgly[] = "s/berebere/tururu/pararapaparaduda/";
	char host[50];
	char tail[50];

	printf("Host: %s\nTail: %s\n", host, tail);
	splitUrl(theGood,host, tail);
	printf("Host: %s\nTail: %s\n", host, tail);

	splitUrl(theBad,host, tail);
	printf("Host: %s\nTail: %s\n", host, tail);

	splitUrl(theUgly,host, tail);
	printf("Host: %s\nTail: %s\n", host, tail);

	printf("TheGood: %s; TheBad: %s; TheUgly: %s\n", theGood,theBad,theUgly);

	return 0;
}
