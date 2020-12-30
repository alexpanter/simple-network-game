/*
 * Server
 */

#include "csapp.h"
#include "protocol.hpp"
#include <cassert>
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <cstring>
#include <atomic>

//
// PLAYER INFO
struct Player
{
	float posX;
	float posY;
	bool is_alive;
	bool is_ready;
};

//
// CLIENT INFO
class RespondMessage
{
public:
	RespondMessage(const char* message)
	{
		mMessageLength = strlen(message);
		assert(mMessageLength < Protocol::kMaxMessageLength);
		mpMessage = (char*)malloc(sizeof(char) * Protocol::kMaxMessageLength);
		strcpy(mpMessage, message);
	}
	~RespondMessage()
	{
		delete mpMessage;
	}
	char* GetMessage() { return mpMessage; }
	size_t GetMessageLength() { return mMessageLength; }
private:
	char* mpMessage;
	size_t mMessageLength;
};

struct Client
{
	// A connected client can request creation of a player, and the game
	// cannot start until all connected clients have done so.
	Player client_player;
	bool player_created;

	int connfd;
	volatile std::atomic_bool client_connected;
	pthread_t receive_tid;
	pthread_t respond_tid;
	pthread_mutex_t client_mutex;

	std::queue<std::shared_ptr<RespondMessage>> message_queue;
};

//
// CLIENT SETUP AND SYNCHRONIZATION
constexpr int kMaxClients = 4;
std::vector<Client*> connected_clients;
pthread_mutex_t connected_clients_mutex;
pthread_cond_t client_cond_respond;


//
// SERVER SETUP
void InitServer()
{
	int status = pthread_mutex_init(&connected_clients_mutex, nullptr);
	assert(status == 0);
}


//
// HANDLE CLIENT ACTIONS
bool AddPlayer(Client* client)
{
	// pthread_mutex_lock(&mutex);
	// if (players.size() >= kMaxPlayers)
	// {
	// 	pthread_mutex_unlock(&mutex);
	// 	return false;
	// }
	// auto it = players.emplace_back();
	// it.posX = it.posY = 0.0f;
	// it.is_alive = true;
	// it.is_ready = false;
	// pthread_mutex_unlock(&mutex);
	return true;
}

void MovePlayer(Player* player, float dirX, float dirY)
{
	// pthread_mutex_lock(&mutex);
	// pthread_mutex_unlock(&mutex);
}


void* ClientRespondThread(void* clientPtr)
{
	Client* client = (Client*)clientPtr;
	int error_tolerance = 5; // max # connection errors that may occur
	printf("Starting respond thread for client[%i]\n", client->connfd);
	while (client->client_connected && error_tolerance > 0)
	{
		pthread_mutex_lock(&client->client_mutex);
		while (client->client_connected && client->message_queue.empty()) {
			printf("respond thread for client[%i] before wait\n", client->connfd);
			pthread_cond_wait(&client_cond_respond, &client->client_mutex);
			printf("respond thread for client[%i] after wait\n", client->connfd);
		}
		printf("respond thread awoken by broadcast, send to client[%i]\n", client->connfd);
		if (!client->client_connected) {
			pthread_mutex_unlock(&client->client_mutex);
			break;
		}
		auto msg = client->message_queue.front();
		client->message_queue.pop();
		printf("respond thread for client[%i] will send message \"%s\"\n",
			   client->connfd, msg->GetMessage());

		// send message to client
		ssize_t write_status = rio_writen(client->connfd, msg->GetMessage(),
		                                  msg->GetMessageLength());
		if(write_status < 1)
		{
			printf("Some error occurred: Could not write to client!");
			error_tolerance--;
		}

		pthread_mutex_unlock(&client->client_mutex);
	}
	return nullptr;
}

void* ClientReceiveThread(void* clientPtr)
{
	Client* client = (Client*)clientPtr;
	int error_tolerance = 5; // max # connection errors that may occur

	// storage buffer for received client requests
	char buf[Protocol::kMaxMessageLength];
	memset(buf, 0, Protocol::kMaxMessageLength);
	rio_t rio;
	rio_readinitb(&rio, client->connfd);


	while(error_tolerance > 0)
	{
		ssize_t read_status = rio_readlineb(&rio, buf, Protocol::kMaxMessageLength);
		if(read_status <= 0)
		{
			// TODO: Will this work?
			error_tolerance--;
			continue;
		}
		printf("Received message \"%s\" from client[%i]\n", buf, client->connfd);

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
			error_tolerance--;
			printf("Client[%i]: empty message received\n", client->connfd);
			memset(buf, 0, read_status);
			continue;
		}

		// for proper string matching we remove trailing newline
		if(buf[read_status] == '\n') buf[read_status] = '\0';
		if(buf[read_status - 1] == '\n') buf[read_status - 1] = '\0';
		printf("Converted to \"%s\"\n", buf);

		// parsing client request
		float moveX, moveY;
		std::shared_ptr<RespondMessage> response = nullptr;

		if (strcmp(buf, "START") == 0)
		{
			printf("client[%i] requested start\n", client->connfd);
			response = std::make_shared<RespondMessage>("GAME_START\n");
		}
		else if (strcmp(buf, "PAUSE") == 0)
		{
			printf("client[%i] requested pause\n", client->connfd);
			response = std::make_shared<RespondMessage>("GAME_PAUSE\n");
		}
		else if (strcmp(buf, "QUIT") == 0)
		{
			printf("client[%i] requested quit\n", client->connfd);
			response = std::make_shared<RespondMessage>("GAME_QUIT\n");
		}
		else if (sscanf(buf, "MOVE %f %f", &moveX, &moveY) == 2)
		{
			printf("client[%i] requested move (%f, %f)\n", client->connfd, moveX, moveY);
			response = std::make_shared<RespondMessage>("PLAYER_MOVE\n");
		}
		else
		{
			printf("client[%i]: unrecongnized command: \"%s\"\n", client->connfd, buf);
			response = std::make_shared<RespondMessage>("INVALID_COMMAND\n");
		}

		pthread_mutex_lock(&connected_clients_mutex);
		for (auto& client_ptr : connected_clients)
		{
			pthread_mutex_lock(&client_ptr->client_mutex);
			printf("Sending message \"%s\" to client[%i]\n", response->GetMessage(),
				   client_ptr->connfd);
			client_ptr->message_queue.push(response);
			pthread_mutex_unlock(&client_ptr->client_mutex);
		}
		pthread_cond_broadcast(&client_cond_respond);
		pthread_mutex_unlock(&connected_clients_mutex);

		memset(buf, 0, read_status);
	}

	client->client_connected.store(false);
	return NULL;
}


int main(int argc, char **argv)
{
	struct sockaddr_storage clientaddr;
	char client_hostname[MAXLINE], client_port[MAXLINE];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	int listenfd = Open_listenfd(argv[1]); // TODO: error checking
	InitServer();
	printf("Server listening on port %s...\n", argv[1]);

	// Initial loop - wait for all players to join
	const int temp_allowed_connections = 2; // just to get things working initially
	int connections = 0;
	while (connections < temp_allowed_connections)
	{
		socklen_t clientlen = sizeof(struct sockaddr_storage);
		int new_connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);

		// Create client object to handle connection
		Client* new_client = new Client();
		new_client->connfd = new_connfd;
		new_client->client_connected.store(true);
		connected_clients.push_back(new_client);

		// Spawn two threads for each connected client, one for receiving requests
		// and one for sending back responses.
		Pthread_create(&new_client->receive_tid, nullptr, ClientReceiveThread, new_client);
		Pthread_create(&new_client->respond_tid, nullptr, ClientRespondThread, new_client);

		// Print debug information about connected client
		Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
		            client_port, MAXLINE, 0);
		printf("Connected to client (%s, %s) via threads (recv: %lu, resp: %lu)\n",
		       client_hostname, client_port, new_client->receive_tid,
		       new_client->respond_tid);

		connections++;
	}

	// Game loop - play game
	bool game_running = true;
	printf("Game running...\n");
	while (game_running)
	{

	}

	// Cleanup - wait for all receive and respond threads to finish
	for (auto& client_ptr : connected_clients)
	{
		void* thread_return_status;
		Pthread_join(client_ptr->receive_tid, &thread_return_status);
		Pthread_join(client_ptr->respond_tid, &thread_return_status);

		delete client_ptr;
	}

	printf("Closing server socket...");
	Close(listenfd);
}
