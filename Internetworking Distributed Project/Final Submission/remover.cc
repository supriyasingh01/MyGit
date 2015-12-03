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

#define MYPORT "4955"    // the port users will be connecting to

#define MAXBUFLEN 100

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void modify(char buf[100])
{
	//rcv[i].protocol, rcv[i].srcPort, rcv[i].dstPort, rcv[i].srcIP, rcv[i].dstIP
	char *proto = strtok(buf,"#");
	char *srcport = strtok(NULL, "#");
	char *dstport = strtok(NULL, "#");
	char *srcip = strtok(NULL, "#");
	char *dstip = strtok(NULL, "#");
	char command[200];
	strcpy(command, "sudo ovs-ofctl mod-flows br-int nw_src=");
	strcat(command, srcip);
	strcat(command, ",nw_dst=");
	strcat(command, dstip);
	switch(atoi(proto))
	{
		case 6:
		strcat(command, ",tcp,");
		break;
		
		case 1:
		strcat(command, ",icmp,");
		break;
		
		case 17:
		strcat(command, ",tcp,");
		break;
	}
	strcat(command, "action=output:0");
	
	printf("Command to run is %s\n",command);
	FILE *pf = popen(command, "r");
	
	memset(command, 0, sizeof(command));
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
	while(1){
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("recvfrom");
			exit(1);
		}

		buf[numbytes] = '\0';
		//printf("listener: packet contains \"%s\"\n", buf);
		modify(buf);
	}
    close(sockfd);

    return 0;
}

