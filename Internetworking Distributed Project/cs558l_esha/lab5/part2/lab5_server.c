//SERVER//SENDING FILE
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
#include <sys/time.h>
#define MAXDATALEN 15120
#define PORT "4950"    // the port users will be connecting to

struct packet {
   
    char data[MAXDATALEN];
    int fileSize;
	int packetNo;
	int bytes_in_msg;
   
};

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
	struct packet pckt;
	int f;
	struct timeval tim;
	double t1;
    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1],PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

	//SENDING FILE 
	FILE* file ;
	file = fopen (argv[2],"r");
	struct stat st;//for size of file
    stat(argv[2], &st);
	
	//filling up message structure
    pckt.fileSize = st.st_size;
    printf("filesize is:%d\n",pckt.fileSize);
    
    //getting data from file 
    printf("File contains:\n");
    for(f=0; f< pckt.fileSize; f++)
	{					   
		pckt.data[f]=fgetc(file);
//		printf("%c",pckt.data[f]);
								
	}
	
	gettimeofday(&tim,NULL);
	t1=(tim.tv_sec*1000000.0)+(tim.tv_usec);
	printf("%lf microsecs\n",t1);

    if ((numbytes = sendto(sockfd,&pckt,sizeof(struct packet) , 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
	gettimeofday(&tim,NULL);
	t1=(tim.tv_sec*1000000.0)+(tim.tv_usec);
	printf("%lf microsecs\n",t1);

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}

