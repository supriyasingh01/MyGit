#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <pthread.h> 

#define PORT "5000"	// the port users will be connecting to
#define MAXDATALEN 50000
#define NUM_THREADS 100
#define MAXSOCKETS 10

struct message {
   
    int data_type;            // 1=CONNECT,2=data
    char data[MAXDATALEN];
    int fileSize;
    //int childpid;
	int packetNo;
	int bytes_in_msg;
   
};

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* makeNewSocket(void* socketCount)
{		
		int sockfd1[MAXSOCKETS];
		struct addrinfo hints, *servinfo, *p;
	    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd1[*(int*)socketCount+1] = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("RECEIVER: socket");
            continue;
        }

        //if (bind(sockfd1[*(int*)socketCount+1], p->ai_addr, p->ai_addrlen) == -1) {
            //close(sockfd1[*(int*)socketCount+1]);
            //perror("RECEIVER: bind");
            //continue;
        //}

        //break;
    //}

    //if (p == NULL) {
        //fprintf(stderr, "2RECEIVER: failed to bind socket\n");
        ////return 2;
    //}

    freeaddrinfo(servinfo);
    
//return sockfd[*(int*)socketCount];    
}   
