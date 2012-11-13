/*
 * Example of client using TCP protocol.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>

#define NUM_CONNS 4

// You may/may not use pthread for the client code. The client is communicating with
// the server most of the time until he recieves a "GET <file>" request from another client.
// You can be creative here and design a code similar to the server to handle multiple connections.

FILE * getFiles(){
	return popen("ls sharedFiles/", "r");
}

int main(int argc, char *argv[]) {

	char s[1024 * 4];

	char * pch;
	char filePath[1000] = "";
	char tempFileInfo[1100];
	char allFileInfo[1024 * 4];
	char fileSize[30];
	struct stat st;
	fd_set connset;

	if(argc != 5) {
		printf("Usage: ./clientFD <HOST> <PORT_NUMBER> <COMMA ',' DELINEATED FILE LIST> <USERNAME>\n");
		exit(1);
	}

	struct addrinfo hints, *res;
	int sockfd;

	// first, load up address structs with getaddrinfo():
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(argv[1], argv[2], &hints, &res);

	// make a socket:

	if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket()");
		exit(1);
	}


	// connect it to the address and port we passed in to getaddrinfo():

	if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect()");
		exit(1);
	}
	

	bzero(allFileInfo, sizeof(allFileInfo));
	pch = strtok(argv[3], ",");

	while(pch != NULL){

		bzero(filePath, sizeof(filePath));
		bzero(fileSize, sizeof(fileSize));
		bzero(tempFileInfo, sizeof(tempFileInfo));

		strcat(filePath, "sharedFiles/");
		strcat(filePath, pch);

		stat(filePath, &st);
		sprintf(fileSize, "%d", st.st_size );
		


		strcat(tempFileInfo, pch);
		strcat(tempFileInfo, "/");
		strcat(tempFileInfo, fileSize);
		strcat(tempFileInfo, "/");
		strcat(tempFileInfo, argv[4]);

		strcat(allFileInfo, tempFileInfo);
		strcat(allFileInfo, ";");

		printf("File info string: %s\n", allFileInfo);

		pch = strtok(NULL, ",");
	}

	if(send(sockfd, allFileInfo, sizeof(allFileInfo), NULL) < 0) {
		perror("send()");
		exit(1);
	}

	while(1) {
		
		printf("clientFD $: ");

		FD_ZERO(&connset);
		FD_SET(sockfd,&connset); /* add sockfd to connset */
	    FD_SET(STDIN_FILENO,&connset); /* add STDIN to connset */	

	    if(select(NUM_CONNS + 1, &connset,NULL,NULL,NULL) < 0){
        	fprintf(stdout, "select() error\n");
        	exit(0);
    	}  

    	if( FD_ISSET(STDIN_FILENO, &connset) ) {
    		
    		gets(s);

    		if(send(sockfd, s, sizeof(s), NULL) < 0) {
				perror("send()");
				exit(1);
			}
    	}
		
		if (FD_ISSET(sockfd, &connset)) {
			
			if(recv(sockfd, s, sizeof(s), 0) < 0) {
				perror("recv()");
				exit(1);
			}

			printf("Recieved from server: %s\n", s);
		
		}
		
	}

	return 0;
}
