/*
 * echoservert.c - A concurrent echo server using threads
 */

#include "csapp.h"
#include "protocol.hpp"
#include <cassert>
#include <vector>

void echo(int connfd);
void *thread(void *vargp);

enum class GameStateType
{
	not_started
};

class Player
{
private:
public:
};

class Game
{
private:
	pthread_mutex_t mutex;
	std::vector<Player> players;

public:
	Game()
	{
		int status = pthread_mutex_init(&mutex, nullptr);
		assert(status == 0);
	}
	void AddPlayer()
	{
		pthread_mutex_lock(&mutex);
		players.emplace_back();
		pthread_mutex_unlock(&mutex);
	}
};

void *thread(void *vargp)
{
	int connfd = *((int *)vargp);
	Pthread_detach(pthread_self());
	Free(vargp);

	// process
	char buf[Protocol::kMaxMessageLength];
	memset(buf, 0, Protocol::kMaxMessageLength);
	rio_t rio;
	rio_readinitb(&rio, connfd);

	int terminate = 0;
	int max_empty_msg_count = 20;
	while(!terminate)
	{
		if(max_empty_msg_count <= 0) break;

		//rio_readinitb(&rio, connfd);
		printf("new iteration\n");
		ssize_t read_status = rio_readlineb(&rio, buf, Protocol::kMaxMessageLength);
		if(read_status <= 0) break;

		//printf("WAS RECEIVED: '%s'\n", buf);

		// check for empty messages
		int only_whitespace = 1;
		for(int i = 0; i < read_status; i++)
		{
			if((buf[i] != '\n') && (buf[i] != '\0') && (buf[i] != ' ') && (buf[i] != '\r'))
			{
				only_whitespace = 0;
				break;
			}
		}
		if(only_whitespace)
		{
			max_empty_msg_count--;
			printf("empty message received\n");
			memset(buf, 0, read_status);
			continue;
		}

		// for proper string matching we remove trailing newline
		if(buf[read_status] == '\n') buf[read_status] = '\0';
		if(buf[read_status - 1] == '\n') buf[read_status - 1] = '\0';

		// parsing client request
		float moveX, moveY;
		char outbuf[Protocol::kMaxMessageLength];
		ssize_t write_status = 0;

		if (strcmp(buf, "START") == 0)
		{
			printf("client[%i] requested start\n", connfd);
			strncpy(outbuf, "GAME_START\n", Protocol::kMaxMessageLength);
		}
		else if (strcmp(buf, "PAUSE") == 0)
		{
			printf("client[%i] requested pause\n", connfd);
			strncpy(outbuf, "GAME_PAUSE\n", Protocol::kMaxMessageLength);
		}
		else if (strcmp(buf, "QUIT") == 0)
		{
			printf("client[%i] requested quit\n", connfd);
			strncpy(outbuf, "GAME_QUIT\n", Protocol::kMaxMessageLength);
		}
		else if (sscanf(buf, "MOVE %f %f", &moveX, &moveY) == 2)
		{
			printf("client[%i] requested move (%f, %f)\n", connfd, moveX, moveY);
			strncpy(outbuf, "PLAYER_MOVE\n", Protocol::kMaxMessageLength);
		}
		else
		{
			printf("client[%i]: unrecongnized command: \"%s\"\n", connfd, buf);
			strncpy(outbuf, "INVALID_COMMAND\n", Protocol::kMaxMessageLength);
		}

		write_status = rio_writen(connfd, outbuf, strlen(outbuf));
		if(write_status < 1)
		{
			printf("Some error occurred: Could not write to client!");
			max_empty_msg_count = 0;
			continue;
		}

		memset(buf, 0, read_status);
	}

	printf("Closing connection to thread %lu", pthread_self());
	Close(connfd);
	return NULL;
}

int main(int argc, char **argv)
{
	int listenfd, *connfdp;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;
	char client_hostname[MAXLINE], client_port[MAXLINE];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	listenfd = Open_listenfd(argv[1]);
	printf("Server listening on port %s...\n", argv[1]);

	while (1) {
		clientlen=sizeof(struct sockaddr_storage);
		connfdp = (int*)Malloc(sizeof(int));
		*connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
		Pthread_create(&tid, NULL, thread, connfdp);
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
		            client_port, MAXLINE, 0);
		printf("Connected to client (%s, %s) via thread %lu\n",
		       client_hostname, client_port, (long) tid);
	}
}
