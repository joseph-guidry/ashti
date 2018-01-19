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
#define MAX_BUFFER 256
#define PATH_LENGTH 100

int http_server(void);
void http_response(int connection_id);
void run_cgi_script(int connection_id, char * local_path);

int connections[MAX_CONNECTS];
char root_path[PATH_LENGTH];

int main(int argc, char **argv)
{
	struct sockaddr_in6 clntAddr;
	int sockfd;
	int ret_val;

	unsigned int addrLen;

	char buffer[128]; // FOR printing out the IPv6 address
	int connection_id = 0;

	if (argc != 2 )
	{
		perror("Invalid Usage");
		exit(1);
	}

	if (argv[1][0] != '/')
		snprintf(root_path, PATH_LENGTH, "%s/%s", getenv("PWD"), argv[1] );
	else
		strncpy(root_path, argv[1], PATH_LENGTH);
	
	printf("Path to files [%s]\n", root_path);
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
			if ( (ret_val = fork()) < 0  )
			{
				send(connections[connection_id], "HTTP/1.1 500 INTERNAL SERVER ERROR\r\n", 36, 0);
			}
			else if (  ret_val == 0)
			{
				printf("child has process and will respond\n");
				http_response(connection_id);
				exit(0);
			}
		}
		while(connections[connection_id] != -1 )
			connection_id = (connection_id + 1) % MAX_CONNECTS;
	}
	free(root_path);
	return 0;
}


void http_response(int connection_id)
{
	printf("Recevied connection id = %d, path = %s \n", connection_id, root_path);
	int read_pipe[2], write_pipe[2];
	char http_request_buffer[MAX_BUFFER], *http_header[3], data[MAX_BUFFER], local_path[PATH_LENGTH];
	int byte_recv, docfd, read_size;

	pid_t childpid;

	memset(http_request_buffer, (int)'\0', MAX_BUFFER);

	byte_recv = recv(connections[connection_id], http_request_buffer, MAX_BUFFER - 1, 0);
	http_request_buffer[byte_recv] = '\0';

	printf("Doing a HTTP response?\n");
	if ( byte_recv < 0)
	{
		perror("recv () error failure");
	}
	else if ( byte_recv == 0)
	{
		perror("recv () error terminated");
	}
	else
	{
		printf("remaining http request\n\n%s\n\n\n\n", http_request_buffer);
		http_header[0] = strtok(http_request_buffer, " \n\t");
		printf("Item 1 = [%s]\n", http_header[0]);

		if (strcmp(http_header[0], "GET") == 0)
		{
			printf("GOT A GET REQUEST\n");
			http_header[1] = strtok(NULL, " \n\t");
			http_header[2] = strtok(NULL, " \n\t");
			printf("Looking for [%s]\n", http_header[1]);

			/* CHECK FOR VALID PROTOCOL */
			if (strncmp(http_header[2], "HTTP/1.1", 8) != 0 && strncmp(http_header[2], "HTTP/1.0", 8) != 0 )
			{
				printf("here\n");
				write(connections[connection_id], "HTTP/1.1 400 Bad Request\n", 25);
			}
			else
			{
				printf("or here\n");				
				if (strncmp(http_header[1], "/\0", 2) == 0)
					http_header[1] = strdup("/index.html");

				if (strstr(http_header[1], ".cgi") != NULL)
				{
					send(connections[connection_id], "HTTP/1.1 200 OK\r\n", 17, 0);
					snprintf(local_path, PATH_LENGTH, "%s%s%s", root_path, "/cgi-bin", http_header[1]);
					printf("CGI Script\n");
					// FORK() then pipe the return to read from pipe to buffer, then write from buffer to socket?
				}
				else if (strstr(http_header[1], ".html") != NULL)
				{
					/* This is under the www file ( Move this to its own function? ) */
					snprintf(local_path, PATH_LENGTH, "%s%s%s", root_path, "/www", http_header[1]);
					printf("new path for www files = [%s]\n", local_path);
					if( (docfd = open(local_path, O_RDONLY)) != -1)
					{
						send(connections[connection_id], "HTTP/1.1 200 OK\r\n", 17, 0);
						send(connections[connection_id], "Content-Type: text/html\r\n\r\n", 27, 0);

						while( (read_size = read(docfd, data, MAX_BUFFER -1 )) > 0)
						{
							printf("writing\n");
							data[MAX_BUFFER] = '\0';
							write(connections[connection_id], data, read_size);
						}
					}
					else
					{
						write(connections[connection_id], "HTTP/1.1 404 Not Found\n", 23);
					}
				}
				else
				{
					printf("extensions that are not handled by this program ... log it maybe\n");
				}
			}
		}
		else if (strcmp(http_header[0], "POST") == 0)
		{
			printf("GOT A POST REQUEST\n");
			http_header[1] = strtok(NULL, " \n\t");
			http_header[2] = strtok(NULL, " \n\t");
			printf("Looking for [%s]\n", http_header[1]);

			/* CHECK FOR VALID PROTOCOL */
			if (strncmp(http_header[2], "HTTP/1.1", 8) != 0 && strncmp(http_header[2], "HTTP/1.0", 8) != 0 )
			{
				printf("here\n");
				write(connections[connection_id], "HTTP/1.1 400 Bad Request\n", 25);
			}
			else
			{
				printf("or here\n");				
				if (strncmp(http_header[1], "/\0", 2) == 0)
					http_header[1] = strdup("/index.html");

				/* 
					SEND a meta-data response with
					- Date/Content-Type/encoding/length/ and other options? 
				*/
				/* If file ends in .cgi -> run CGI script ( Move this to its own function? ) */
				if (strstr(http_header[1], ".cgi") != NULL)
				{
					send(connections[connection_id], "HTTP/1.1 200 OK\r\n", 17, 0);
					send(connections[connection_id], "Content-Type: text/html\r\n\r\n", 27, 0);
					snprintf(local_path, PATH_LENGTH, "%s%s", root_path, http_header[1]);
					printf("CGI Script [%s]\n", local_path);
					if ( pipe(read_pipe) < 0 || pipe(write_pipe) < 0)
					{
						printf("PIPE FAILED");
					}


					if ( (childpid = fork()) < 0)
					{
						perror("fork() fail");
					}
					else if (childpid == 0)
					{
						printf("were going to run a cgi script\n");
						close(read_pipe[0]);
						close(write_pipe[1]);

						dup2(write_pipe[0], 0); /* the read end of the first pipe is p1[0] -- copy it to stdin */
						dup2(read_pipe[1], 1);  /* the read end of the first pipe is p1[0] -- copy it to stdin */
						//printf("execute file %s : %s \n", local_path, strrchr(local_path, '/') + 1); 
						execl(local_path,  strrchr(local_path, '/') + 1, NULL, NULL); /* this is where to put the other HTTP stuff */
						perror("execl of date failed");
						exit(-44);
					}
					else
					{
						close(write_pipe[0]);
						close(read_pipe[1]);
				
						printf("can we try to write this\n");
						while( (read_size = read(read_pipe[0], data, MAX_BUFFER -1 )) > 0)
						{
							printf("writing\n");
							data[MAX_BUFFER] = '\0';
							write(connections[connection_id], data, read_size);
						}
					}
				}
				else
				{
					printf("extensions that are not handled by this program ... log it maybe\n");
				}
			}
		}
		else
		{
			printf("500 Error\n");
		}
	}
	shutdown(connections[connection_id], SHUT_RDWR);
	close(connections[connection_id]);
	connections[connection_id] = -1;
}

int http_server(void)
{
	int sockfd, option = 1;
	struct sockaddr_in6 srvAddr;

	unsigned short srvPort;
	srvPort = getuid();	

	if ((sockfd = socket(PF_INET6, SOCK_STREAM, 0) ) < 0)
	{
		perror("socket () error");
		exit(1);
	}

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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
