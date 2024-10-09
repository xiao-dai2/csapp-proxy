#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh 
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */

void doit(int fd);
void read_requesthdrs(rio_t *rp,char *content,char *hostname);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	doit(connfd);                                             //line:netp:tiny:doit
	Close(connfd);                                            //line:netp:tiny:close
    }
}
/* $end tinymain */

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
void doit(int fd) 
{
    int count;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char content[MAXLINE];
    char hostname[MAXLINE];
    char port[20]="80";
    char temp[MAXLINE];
    char * p = NULL,*ptr=NULL;
    rio_t rio,rioserver;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    strcpy(version,"HTTP/1.0");
    sprintf(buf,"%s %s %s",method,uri,version);
   // printf("%s", buf);//
    read_requesthdrs(&rio,content,hostname);
   // printf("before:%s",content);//
    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }         
    if(!strstr(uri,"http:")){ // no hostname 
       
        strcpy(temp,content);
        sprintf(content,"%s\r\n%s",buf,temp);
        strcat(content,user_agent_hdr);
        strcat(content,"Connection: close\r\n");
        strcat(content,"Proxy-Connection: close\r\n");
        strcat(content,"\r\n");
    }
    else{
        p  = index(uri,'/');
        p += 2;
        ptr = p;
        p  = index(p,'/');
        *p = '\0';
        strcpy(hostname,ptr);
        printf("hostname:%s\n",hostname);
        *p = '/';
        strcpy(uri,p);
        if(hostname[strlen(hostname)-1]>='0'&&hostname[strlen(hostname)-1]<='9'){
            p = index(hostname,':');
            strcpy(port,p+1);
            *p = '\0';
        }
        sprintf(buf,"%s %s %s",method,uri,version);
        strcpy(temp,content);
        sprintf(content,"Host: %s\r\n%s",hostname,temp);
        strcpy(temp,content);
        sprintf(content,"%s\r\n%s",buf,temp);
        strcat(content,user_agent_hdr);
        strcat(content,"Connection: close\r\n");
        strcat(content,"Proxy-Connection: close\r\n");
        strcat(content,"\r\n");
    }
    printf("after:\n%s",content);//
    int serverfd = Open_clientfd(hostname,port);
    Rio_readinitb(&rioserver,serverfd);
    Rio_writen(serverfd,content,sizeof(content));
    while ((count = Rio_readlineb(&rioserver,buf,MAXLINE-1))!=0)
    {
        Rio_writen(fd,buf,sizeof(buf));
    
    }
    Close(serverfd);

                                                
}
/* $end doit */

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp,char *content,char *hostname) 
{
    char buf[MAXLINE];
    buf[0] = '\0';
    char *p;
    Rio_readlineb(rp, buf, MAXLINE);
    strcpy(content,buf);
    if(strstr(buf,"Host")){
        p = index(buf,':');
        p++;
        while(*p==' ') p++;
        strcpy(hostname,p);
    }
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	Rio_readlineb(rp, buf, MAXLINE);
	strcat(content,buf);
    if(strstr(buf,"Host")){
        p = index(buf,':');
        p++;
        while(*p==' ') p++;
        strcpy(hostname,p);
    }
    }
    content[strlen(content)-2] = '\0';
   // printf("content:%s",content);//
  
    return;
}
/* $end read_requesthdrs */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */

