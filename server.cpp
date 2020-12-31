/*
 * Server
 */

#include "csapp.h"
#include "protocol.hpp"
#include "game.hpp"

#include <cassert>
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include <cstring>
#include <atomic>


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
std::vector<Client*> connected_clients;
pthread_mutex_t connected_clients_mutex;
pthread_cond_t client_cond_respond;

//
// GAME DATA
Game* game;


//
// SERVER SETUP
void InitServer()
{
	int status = pthread_mutex_init(&connected_clients_mutex, nullptr);
	assert(status == 0);
}
void InitGame()
{
	game = new Game();
}


//
// HANDLE CLIENTS
void* ClientRespondThread(void* clientPtr)
{
	Client* client = (Client*)clientPtr;
	int error_tolerance = 5; // max # connection errors that may occur
	printf("Starting respond thread for client[%i]\n", client->connfd);

	while (client->client_connected && error_tolerance > 0)
	{
		pthread_mutex_lock(&client->client_mutex);
		while (client->client_connected && client->message_queue.empty()) {
			pthread_cond_wait(&client_cond_respond, &client->client_mutex);
		}
		//printf("respond thread awoken by broadcast, send to client[%i]\n", client->connfd);
		if (!client->client_connected) {
			pthread_mutex_unlock(&client->client_mutex);
			break;
		}
		auto msg = client->message_queue.front();
		client->message_queue.pop();
		// printf("respond thread for client[%i] will send message \"%s\" with length %lu\n",
		// 	   client->connfd, msg->GetMessage(), msg->GetMessageLength());

		// send message to client
		ssize_t write_status = rio_writen(client->connfd, msg->GetMessage(),
		                                  msg->GetMessageLength());
		if(write_status < 1)
		{
			printf("Some error occurred: Could not write to client!");
			error_tolerance--;
		}
		// printf("respond thread for client[%i] has sent message \"%s\" with length %lu\n",
		// 	   client->connfd, msg->GetMessage(), msg->GetMessageLength());

		pthread_mutex_unlock(&client->client_mutex);
	}

	printf("Terminating respond thread for client[%i]\n", client->connfd);
	return nullptr;
}

void* ClientReceiveThread(void* clientPtr)
{
	Client* client = (Client*)clientPtr;
	printf("Starting respond thread for client[%i]\n", client->connfd);

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

		// check for empty messages
		bool only_whitespace = true;
		for(int i = 0; i < read_status; i++)
		{
			if((buf[i] != '\n') && (buf[i] != '\0') && (buf[i] != ' ') && (buf[i] != '\r'))
			{
				only_whitespace = false;
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

		// parsing client request
		float moveX, moveY;
		std::shared_ptr<RespondMessage> response = nullptr;
		bool broadcast_response = false;

		if (strcmp(buf, Protocol::CLIENT_REQUEST_START) == 0)
		{
			printf("client[%i] requested start\n", client->connfd);
			game->PlayerSetReady(&client->client_player);

			if (game->TryStartGame())
			{
				printf("ACTION: Game can be started!\n");
				response = std::make_shared<RespondMessage>
					(Protocol::SERVER_RESPONSE_START);
				broadcast_response = true;
			}
			else
			{
				printf("ACTION: Game can not be started!\n");
			}
		}
		else if (strcmp(buf, Protocol::CLIENT_REQUEST_TOGGLE_PAUSE) == 0)
		{
			printf("client[%i] requested pause/unpause\n", client->connfd);
			bool take_action = game->PauseUnpauseGame(&client->client_player);

			if (take_action && game->GetGameState() == GameStateType::running)
			{
				printf("ACTION: Game will be paused!\n");
				response = std::make_shared<RespondMessage>
					(Protocol::SERVER_RESPONSE_PAUSE);
			}
			else if (take_action && game->GetGameState() == GameStateType::paused)
			{
				printf("ACTION: Game will be unpaused!\n");
				response = std::make_shared<RespondMessage>
					(Protocol::SERVER_RESPONSE_UNPAUSE);
			}
			else
			{
				printf("ACTION: No pause/unpause action will be taken!\n");
			}
			broadcast_response = take_action;
		}
		else if (strcmp(buf, Protocol::CLIENT_REQUEST_QUIT) == 0)
		{
			printf("client[%i] requested quit\n", client->connfd);
			bool take_action = game->PlayerQuit(&client->client_player);

			if (take_action)
			{
				printf("ACTION: Client will quit!\n");
				response = std::make_shared<RespondMessage>
					(Protocol::SERVER_RESPONSE_END_GAME);
				broadcast_response = take_action;
			}
			else
			{
				printf("ACTION: Client will not quit!\n");
			}
		}
		else if (sscanf(buf, "CLT_REQ_MOVE %f %f", &moveX, &moveY) == 2)
		{
			printf("client[%i] requested move (%f, %f)\n",
				   client->connfd, moveX, moveY);
			bool should_move = game->MovePlayer(&client->client_player, moveX, moveY);

			if (should_move)
			{
				char outbuf[Protocol::kMaxMessageLength];
				Protocol::CreateMoveResponse(outbuf,
											 client->client_player.player_id,
											 client->client_player.posX,
											 client->client_player.posY);

				response = std::make_shared<RespondMessage>(outbuf);
				broadcast_response = should_move;
				printf("ACTION: Client will move!\n");
			}
			else
			{
				printf("ACTION: Client will not move!\n");
			}
		}
		else
		{
			printf("client[%i]: unrecongnized command: \"%s\"\n", client->connfd, buf);
			printf("ACTION: No action will be taken!\n");
		}

		if (broadcast_response)
		{
			// Pushing server response to each client connection's message queue,
			// then signals each client's respond thread to send the message to
			// its client.
			pthread_mutex_lock(&connected_clients_mutex);
			for (auto& client_ptr : connected_clients)
			{
				pthread_mutex_lock(&client_ptr->client_mutex);
				client_ptr->message_queue.push(response);
				pthread_mutex_unlock(&client_ptr->client_mutex);
			}
			pthread_cond_broadcast(&client_cond_respond);
			pthread_mutex_unlock(&connected_clients_mutex);
		}

		memset(buf, 0, read_status); // TODO: Probably not required.
	}

	printf("Terminating receive thread for client[%i]\n", client->connfd);
	client->client_connected.store(false);

	pthread_mutex_lock(&connected_clients_mutex);
	pthread_cond_broadcast(&client_cond_respond);
	pthread_mutex_unlock(&connected_clients_mutex);

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
	InitGame();
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
		game->AddPlayer(&new_client->client_player);

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
		Close(client_ptr->connfd);

		delete client_ptr;
	}

	printf("Closing server socket...");
	delete game;
	Close(listenfd);
}
