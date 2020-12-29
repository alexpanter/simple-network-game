/*
 * client.cpp
 */
#include "csapp.h"
#include "protocol.hpp"

int main(int argc, char **argv)
{
    int clientfd;
    char* host;
	char* port;
	char buf[Protocol::kMaxMessageLength];
	char server_response[Protocol::kMaxMessageLength];
    rio_t rio;
	rio_t rio_read;
	memset(buf, 0, Protocol::kMaxMessageLength);
	memset(server_response, 0, Protocol::kMaxMessageLength);

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);
	Rio_readinitb(&rio_read, clientfd);
	printf("Connected to server...\n");

    while (Fgets(buf, Protocol::kMaxMessageLength, stdin) != NULL)
    {
		if (buf[0] == '\n') continue;

        ssize_t write_status = rio_writen(clientfd, buf, strlen(buf));
		if (write_status < 1)
		{
			printf("Failed writing to server!\n");
		}

        ssize_t read_status = Rio_readlineb(&rio_read, server_response,
											Protocol::kMaxMessageLength);
		if (read_status < 1)
		{
			printf("Failed reading from server!\n");
		}

        Fputs(server_response, stdout);
        fflush(stdout);
    }
    Close(clientfd);
    exit(0);
}
