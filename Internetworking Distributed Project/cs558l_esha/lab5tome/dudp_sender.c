/*
SENDER
*/
#include "headers.h"


int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	int f;
	struct message m2;

	if (argc != 3) {
		fprintf(stderr,"usage: SENDER hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) { //PORT= Receiver's port no.
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("SENDER: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "SENDER: failed to bind socket\n");
		return 2;
	}
	
	//Sending CONNECTION REQUEST (data_type=1)
	//filling up the message structure
    m2.fileSize =0;
    printf("filesize is:%d\n",m2.fileSize);
    m2.data_type=1;
    printf("data type is:%d\n",m2.data_type);
	
	if ((numbytes = sendto(sockfd, &m2, sizeof(struct message), 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("SENDER: sendto");
		exit(1);
	}
	printf("Sending a CONNECTION request\n");

	//SENDING FILE (data_type=2)
	FILE* file ;
	file = fopen (argv[2],"r");
	struct stat st;//for size of file
    stat(argv[2], &st);
	
	//filling up message structure
    m2.fileSize = st.st_size;
    printf("filesize is:%d\n",m2.fileSize);
    m2.data_type=2;
    printf("data type is:%d\n",m2.data_type);
    
    //getting data from file 
    printf("File contains:\n");
    for(f=0; f< m2.fileSize; f++)
	{					   
		m2.data[f]=fgetc(file);
		printf("%c",m2.data[f]);
								
	}
	
    // Send file size,data,type everything
	if ((numbytes = sendto(sockfd, &m2, sizeof(struct message), 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("SENDER: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("SENDER: sent %d bytes to %s\n", numbytes, argv[1]);
	close(sockfd);

	return 0;
}

