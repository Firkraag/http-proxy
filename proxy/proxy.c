/* $begin tinymain */
/*
 * proxy.c - A simple http proxy.
 */
#include "csapp.h"

//#define THREAD 1
//#define IMAGE 1
//#define MALLOC 1
#define WRITEN

void doit(int fd);
void copy_requesthdrs(rio_t *rp, char *proxy, char *host, char *port, int *image); 
int check_host_port(char *buf, char *host, char *port);
int check_image(char *buf, int *imagep);
int check_content_length(char *buf, int *length);
void copy_resphdrs(rio_t *rp, char *proxy, int *length);
void *thread(void *vargp);
void *sig_thread(void *vargp);
void handler(int sig);

void handler(int sig)
{
	if (sig == SIGPIPE)
	{
		printf("Thread id: %d\n", pthread_self());
		printf("You are SIGPIPE\n");
	}
	return;
}

int main(int argc, char **argv) 
{
    int listenfd, *clientfd, port, clientlen;
    struct sockaddr_in clientaddr;
	pthread_t tid;
	sigset_t oldmask;
	sigset_t mask;


    /* Check command line args */
    if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
    }
	Signal(SIGPIPE, SIG_IGN);
	#ifdef PIPE
	Pthread_create(&tid, NULL, sig_thread, NULL);
	#endif
//	sigemptyset(&mask);
//	sigaddset(&mask, SIGPIPE);
//	pthread_sigmask(SIG_BLOCK, &mask, &oldmask);
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    while (1) {
		clientfd = (int *) Malloc(sizeof(int));
		#ifdef MALLOC
		printf("MALLOC pointer: %p\n", clientfd);
		#endif
		clientlen = sizeof(clientaddr);
		*clientfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
//		printf("connfd = %d\n", connfd);
		Pthread_create(&tid, NULL, thread, (void *) clientfd);
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
	#ifdef THREAD
	printf("method: %s\n", method);
	printf("uri: %s\n", uri);
	printf("version: %s\n", version);
	#endif
	#ifndef IMAGE
	if (strcasestr(uri, ".png") || strcasestr(uri, ".jpeg") || strcasestr(uri, ".jpg") || strcasestr(uri, "gif"))
		return;
	#endif
	location = uri;
	if (location = strstr(uri, "//"))
		location = index(location + 2, '/');
	sprintf(proxy, "%s %s %s\r\n", method, location, version);
    copy_requesthdrs(&rio_client, proxy, host, port, &image);
	#ifndef IMAGE
	if (image)
	{
		printf("No image\n");
		return;
	}
	#endif
	#ifdef DEBUG
	printf("host: %s\tport: %s\n", host, port);
	#endif
	if (host[0] == '\0')
		return;								//line:netp:doit:readrequesthdrs
	if (!strcasecmp(method, "POST"))
	{
		;
	}
	proxyfd = Open_clientfd(host, atoi(port));	
    Rio_readinitb(&rio_proxy, proxyfd);
	#ifdef WRITEN
	printf("Before writen 1\n");
	#endif
	Rio_writen(proxyfd, proxy, strlen(proxy)); 
	#ifdef WRITEN
	printf("After writen 2\n");
	#endif
	#ifdef DEBUG
	printf("%s\n", proxy);
	#endif
	proxy[0] = '\0';
	copy_resphdrs(&rio_proxy, proxy, &content_length);
	#ifdef WRITEN
	printf("Before writen2\n");
	#endif
	Rio_writen(clientfd, proxy, strlen(proxy));
	#ifdef WRITEN
	printf("After writen2\n");
	#endif
	#ifdef DEBUG
	printf("%s\n", proxy); 
	printf("%d\n", content_length);
	#endif
	proxy[0] = '\0';
	inter = (char *) Malloc(content_length);
	#ifdef MALLOC
	printf("MALLOC pointer: %p\n", inter);
	#endif
	Rio_readnb(&rio_proxy, inter, content_length);
	Rio_writen(clientfd, inter, content_length); 
	#ifdef MALLOC
	printf("FREE pointer: %p\n", inter);
	#endif
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
	Pthread_detach(pthread_self());
	int clientfd = *((int *) vargp);
	#ifdef MALLOC
	printf("FREE pointer: %p\n", vargp);
	#endif
	Free(vargp);
	doit(clientfd);
	Close(clientfd);
	return NULL;
}
void *sig_thread(void *vargp)
{
	sigset_t mask, oldmask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGPIPE);
	pthread_sigmask(SIG_UNBLOCK, &mask, &oldmask);
	printf("sig_thread: %d\n", pthread_self());
	while (1)
		;
}
