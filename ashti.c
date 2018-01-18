#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define MAX_CONNECTS 1000
#define LISTEN_QUEUE 5
#define MAX_BUFFER 1024
#define PATH_LENGTH 100 /* SHOULD BE ./www_root ONLY (10 characters) */

int http_server(void);
void http_response(int connection_id);

int connections[MAX_CONNECTS];
char * path;
int main(int argc, char **argv)
{
	struct sockaddr_in6 clntAddr;
	int sockfd;

	unsigned int addrLen;

	char buffer[128];
	char * path= malloc(PATH_LENGTH);
	int connection_id = 0;

;
	if (argc != 2 )
	{
		perror("Invalid Usage");
		exit(1);
	}

	if (argv[1][0] != '/')
		snprintf(path, PATH_LENGTH, "%s/%s", getenv("PWD"), argv[1] );
	else
		path = argv[1];
	
	printf("Path to files [%s]\n", path);
	/* Initialize the bank of connections */
	for(int i = 0; i < MAX_CONNECTS; i++)
	{
		connections[i] = -1;
	}

	sockfd = http_server();

	for(;;)
	{
		addrLen = sizeof(clntAddr);
		if ((connections[connection_id] = accept(sockfd, (struct sockaddr *)&clntAddr, &addrLen)) < 0)
		{
			perror("accept () error");
			exit(1);
		}
		else
		{
			if (inet_ntop(AF_INET6, &clntAddr, buffer, sizeof(buffer)) != NULL)
				printf("Incoming connection from [%s]\n", buffer);
			if ( fork() == 0)
			{
				printf("child has process and will respond\n");
				//http_response(connection_id, path);
				
				exit(0);
			}
		}
		while(connections[connection_id] != -1 )
			connection_id = (connection_id + 1) % MAX_CONNECTS;
		
	}
	return 0;
}


void http_response(int connection_id)
{
	printf("Recevied connection id = %d, path = %s \n", connection_id, path);

	char http_request_buffer[MAX_BUFFER], *http_header[3], data[MAX_BUFFER];
	int byte_recv, docfd, read_size;

	memset(http_request_buffer, (int)'\0', MAX_BUFFER);

	byte_recv = recv(connections[connection_id], http_request_buffer, MAX_BUFFER, 0);

	if ( byte_recv < 0)
	{
		perror("recv () error");
	}
	else if ( byte_recv == 0)
	{
		perror("recv () error");
	}
	else
	{
		http_header[0] = strtok(http_request_buffer, "\n\t");
		printf("Item 1 = [%s]\n", http_header[0]);

		if (strcmp(http_header[0], "GET") == 0)
		{
			printf("GOT A GET REQUEST\n");
		}
		else if (strcmp(http_header[0], "PUT") == 0)
		{
			printf("GOT A PUT REQUEST\n");
		}
		else
		{
			printf("404 ERROR\n");
		}

	}

	close(connections[connection_id]);
	connections[connection_id] = -1;
	
}

int http_server(void)
{
	int sockfd;
	struct sockaddr_in6 srvAddr;

	unsigned short srvPort;
	srvPort = getuid();	

	if ((sockfd = socket(PF_INET6, SOCK_STREAM, 0) ) < 0)
	{
		perror("socket () error");
		exit(1);
	}

	memset(&srvAddr, 0, sizeof(struct sockaddr_in6));
	srvAddr.sin6_family = AF_INET6;
	srvAddr.sin6_flowinfo = 0;
	srvAddr.sin6_addr = in6addr_loopback;
	srvAddr.sin6_port = htons(srvPort);


	if (bind(sockfd, (struct sockaddr *)&srvAddr, sizeof(srvAddr)) < 0 )
	{
		perror("bind () error");
		exit(1);
	}

	if (listen(sockfd, LISTEN_QUEUE) == -1)
	{
		perror("bind () error");
		exit(1);
	}

	printf("Starting Server on Port:[%d]\n", srvPort);

	return sockfd;
}




