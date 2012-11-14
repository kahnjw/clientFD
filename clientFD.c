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
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>

#define NUM_CONNS 4
#define BACKLOG 10 
#define PORT "5002"

// You may/may not use pthread for the client code. The client is communicating with
// the server most of the time until he recieves a "GET <file>" request from another client.
// You can be creative here and design a code similar to the server to handle multiple connections.

int sendFile(char fileName[]);
int recvFile(char * s);
int get(char cmd[], char file[]);

FILE * getFiles(){
	return popen("ls sharedFiles/", "r");
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {

        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {

	char s[1024 * 24];

	char * pch;
	char filePath[1000] = "";
	char tempFileInfo[1100];
	char allFileInfo[1024 * 24];
	char fileSize[30];
	struct stat st;
	fd_set connset;

	int sockfd;

	struct addrinfo hints, *res;
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	if(argc != 5) {
		printf("Usage: ./clientFD <HOST> <PORT_NUMBER> <COMMA ',' DELINEATED FILE LIST> <USERNAME>\n");
		exit(1);
	}

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

	if(send(sockfd, allFileInfo, sizeof(allFileInfo), 0) < 0) {
		perror("send()");
		exit(1);
	}

	while(1) {
		
		FD_ZERO(&connset);
		FD_SET(sockfd,&connset); /* add sockfd to connset */
	    FD_SET(STDIN_FILENO,&connset); /* add STDIN to connset */	

	    if(select(NUM_CONNS + 1, &connset,NULL,NULL,NULL) < 0){
        	fprintf(stdout, "select() error\n");
        	exit(0);
    	} 

		if (FD_ISSET(sockfd, &connset)) {
			
			bzero(s, sizeof(s));
			
			if(recv(sockfd, s, sizeof(s), MSG_WAITALL) < 0) {
				perror("recv()");
				exit(1);
			}

			if(s[0] == '?') {
				pch = strtok(s, "/");
				pch = strtok(NULL, "/");
				
				sendFile(pch);

			} else {

				printf("\nRecieved from server: \n%s", s);
				printf("\n");
			}
		}

		if( FD_ISSET(STDIN_FILENO, &connset) ) {
    		
    		gets(s);
    		char temp[100];
    		char cmdTemp[100];
    		bzero(cmdTemp, sizeof(cmdTemp));

    		strcat(cmdTemp, s);
    		
    		
    		if(send(sockfd, s, sizeof(s), 0) < 0) {
				perror("send()");
				exit(1);
			}

			if(!strcmp(s, "Exit")) {
				close(sockfd);
				exit(0);
			}

			if(get(cmdTemp, temp)) recvFile(temp);
			printf("");
    	}
    	
	}
	return 0;
}


int sendFile(char fileName[]) {

	char filePath[100];
	FILE * file;
	int sockfd, newfd, rv;
	char s[ 1024 * 24 ];

	bzero(s, sizeof(s));

	struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    	printf("Error in getaddrinfo\n");
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("socket()");
        exit(1);
    }

	if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
        exit(1);
    }
 
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

	sin_size = sizeof(their_addr);
    newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

	bzero(filePath, sizeof(filePath));

	strcat(filePath, "sharedFiles/");
	strcat(filePath, fileName);

	if((file = fopen(filePath, "r")) < 0) {
		perror("fopen()");
		exit(1);
	}

	fread(s, 1, 1024 * 24, file);

	if(send(newfd, s, sizeof(s), 0) < 0) {
		perror("send()");
		exit(1);
	}

	// fclose(file);	

	if(send(newfd, s, sizeof(s), 0) < 0) {
        perror("send()");
        exit(1);
    }

    close(newfd);
    close(sockfd);
    
	return 0;
}

int recvFile(char * fileName) {
	
	char filePath[100];
	FILE * file;

	char buffer[1024 * 24];

	int sockfd;

	struct addrinfo hints, *res;
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

    if(getaddrinfo("0", PORT, &hints, &res) != 0){
    	perror("getaddrinfo()");
    	exit(1);
    }

    if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket()");
		exit(1);
	}

	sleep(5);
	
	if(connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect()");
		exit(1);
	}

	printf("About to get message from other client\n");
	if(recv(sockfd, buffer, sizeof(buffer), MSG_WAITALL) < 0) {
		perror("recv()");
		exit(1);
	}

	printf("Message from other client: %s\n", buffer);

	strcat(filePath, "sharedFiles/");
	strcat(filePath, fileName);
	
	if((file = fopen(filePath, "w+")) < 0) {
		perror("fopen()");
		exit(1);
	}

	fwrite(buffer, 1, 1024 * 24, file);

	// fclose(file);
	close(sockfd);
	return 0;
}

int get(char cmd[], char file[]) {
	char * s;

	s = strtok(cmd, " ");

	if(!strcmp(s, "Get")) {
		s = strtok(NULL, " ");
		printf("Get command!\n");

		bzero(file, sizeof(file));
		strcat(file, s);
		return 1;
	}
	return 0;
}