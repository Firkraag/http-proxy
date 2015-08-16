/* $begin tinymain */
/*
 * proxy.c - A simple http proxy.
 */
#include "csapp.h"

void doit(int fd);
void copy_requesthdrs(rio_t *rp, char *proxy, char *host, char *port, int *image); 
int check_host_port(char *buf, char *host, char *port);
int check_image(char *buf, int *imagep);
int check_content_length(char *buf, int *length);
void copy_resphdrs(rio_t *rp, char *proxy, int *length);
void *thread(void *vargp);

int main(int argc, char **argv) 
{
    int listenfd, *clientfd, port, clientlen;
    struct sockaddr_in clientaddr;

    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    while (1) {
		clientfd = (int *) Malloc(sizeof(int));
		clientlen = sizeof(clientaddr);
		*clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
//		printf("connfd = %d\n", connfd);
		thread((void *) clientfd);
    }
	Close(listenfd);
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int clientfd) 
{
    char host[MAXLINE], port[MAXLINE], buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char proxy[MAXBUF];
    rio_t rio_client, rio_proxy;
  	int proxyfd;
	char *location;
	int content_length;
	char *inter;
	int image = 0;

	host[0] = '\0';
	port[0] = '\0';
	proxy[0] = '\0';
    /* Read request line and headers */
    Rio_readinitb(&rio_client, clientfd);
	Rio_readlineb(&rio_client, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasestr(uri, ".png") || strcasestr(uri, ".jpeg") || strcasestr(uri, ".jpg") || strcasestr(uri, "gif"))
		return;
	printf("uri: %s\n", uri);
	location = uri;
	if (location = strstr(uri, "//"))
		location = index(location + 2, '/');
	sprintf(proxy, "%s %s %s\r\n", method, location, version);
    copy_requesthdrs(&rio_client, proxy, host, port, &image);
	if (image)
	{
		printf("No image\n");
		return;
	}
	printf("host: %s\tport: %s\n", host, port);
	if (host[0] == '\0')
		return;								//line:netp:doit:readrequesthdrs
	if (!strcasecmp(method, "POST"))
	{
		;
	}
	proxyfd = Open_clientfd(host, atoi(port));	
    Rio_readinitb(&rio_proxy, proxyfd);
	Rio_writen(proxyfd, proxy, strlen(proxy)); 
	printf("%s\n", proxy);
	proxy[0] = '\0';
	copy_resphdrs(&rio_proxy, proxy, &content_length);
	Rio_writen(clientfd, proxy, strlen(proxy));
	printf("%s\n", proxy); 
	printf("%d\n", content_length);
	proxy[0] = '\0';
	inter = (char *) Malloc(content_length);
	Rio_readnb(&rio_proxy, inter, content_length);
	Rio_writen(clientfd, inter, content_length); 
	Free(inter);
	Close(proxyfd);
}

/*
 * read_requesthdrs - copy HTTP request headers and get host and port
 */
/* $begin copy_requesthdrs */
void copy_requesthdrs(rio_t *rp, char *proxy, char *host, char *port, int *imagep) 
{
    char buf[MAXLINE];
	int exist = 0;
	int image = 0;
	do
	{
		Rio_readlineb(rp, buf, MAXLINE);
		strcat(proxy, buf);
		if (!exist)
			exist = check_host_port(buf, host, port);

		if (image)
			return;
		else
			image = check_image(buf, imagep);
    }  while(strcmp(buf, "\r\n"));          //line:netp:readhdrs:checkterm
    return;
}
/* $end copy_requesthdrs */

int check_image(char *buf, int *imagep)
{
	char *location = index(buf, ':');

	if (location == NULL)
		return 0;
	*location = '\0';
	if (strcasecmp(buf, "accept"))
		return 0;
	if (strcasestr(location + 1, "image"))
	{
		*imagep = 1;
		return	1;
	}
	else
		return 0;
}
int check_host_port(char *buf, char *host, char *port)
{
	char *location = index(buf, ':');
	char *host_port;

	if (location == NULL)
		return 0;
	*location = '\0';
	if (strcasecmp(buf, "host"))
		return 0;
	for (host_port = location + 1; isspace(*host_port); host_port++)
		;
	location = index(host_port, ':');
	if (location == NULL)
	{
		location = index(host_port, '\r');
		*location = '\0';
		strcpy(host, host_port);
		strcpy(port, "80");
		return 1;
	}
	else
	{
		*location = '\0';
		strcpy(host, host_port);
		host_port = location + 1;
		location = index(host_port, '\r');
		*location = '\0';
		strcpy(port, host_port);
		return 1;
	}
}	
void copy_resphdrs(rio_t *rp, char *proxy, int *length)
{
    char buf[MAXLINE];
	int exist = 0;
	do
	{
		Rio_readlineb(rp, buf, MAXLINE);
		strcat(proxy, buf);
		if (!exist)
			exist = check_content_length(buf, length);
    }  while(strcmp(buf, "\r\n"));          //line:netp:readhdrs:checkterm
    return;
}

int check_content_length(char *buf, int *length)
{
	char *location = index(buf, ':');
	char *pos;

	if (location == NULL)
		return 0;
	*location = '\0';
	if (strcasecmp(buf, "Content-length"))
		return 0;
	for (pos = location + 1; isspace(*pos); pos++)
		;
	location = index(location + 1, '\r');
	*location = '\0';
	*length = atoi(pos);
	return 1;
}	

void *thread(void *vargp)
{
	int clientfd = *((int *) vargp);
	Pthread_detach(pthread_self());
	Free(vargp);
	doit(clientfd);
	Close(clientfd);
	return NULL;
}
