/*
** client.c 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/stat.h> 
//#define PORT "5501" // the port client will be connecting to
#define MAXBUFSIZE 255 // max number of bytes we can SEND/get at once

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)

{
    if (sa->sa_family == AF_INET) {

        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

    int sockfd, numbytes; 
    char bufr[MAXBUFSIZE];
    char bufs[MAXBUFSIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    if (argc != 3) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

	memset(&hints, 0, sizeof hints);
	 hints.ai_family = AF_UNSPEC;
	 hints.ai_socktype = SOCK_STREAM;
	 if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		  return 1;
	 }

	 // loop through all the results and connect to the first we can
	 for(p = servinfo; p != NULL; p = p->ai_next) 
	 {
		  if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) 
		  {
				perror("client: socket");
				continue;
		  }
		  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
		  {
				close(sockfd);
				perror("client: connect");
				continue;
		  }
		  break;
	 }

	 if (p == NULL) 
	 {
		  fprintf(stderr, "client: failed to connect\n");
		  return 2;
	 }

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connecting to %s\n",s);
	printf("Please enter the message: ");
    bzero(bufs,256);
    fgets(bufs,255,stdin);

	if (send(sockfd, bufs, sizeof(bufs), 0) == -1)
	{

	 perror("send");

	}
	 if (recv(sockfd, bufr, sizeof(bufr), 0) == -1)
		{
		 perror("send");
		}
		return 0;

}

//lab3c
//
