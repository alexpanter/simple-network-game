/*
 * client.cpp
 */
#include "csapp.h"
#include "protocol.hpp"

// GLEW
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

#include <atomic>
#include "window.hpp"

//
// GAME SYNCHRONIZATION
pthread_mutex_t game_mutex;

//
// SERVER CONNECTION HANDLING
int clientfd;
rio_t response_rio;
pthread_t server_response_tid;
volatile std::atomic_bool game_running;

//
// DRAWING DATA
Windows::WindowedWindow* window;

void* ServerResponseThread(void* /*args*/)
{
	char response[Protocol::kMaxMessageLength];

	while (game_running)
	{
		memset(response, 0, Protocol::kMaxMessageLength);
		ssize_t read_status = Rio_readlineb(&response_rio, response,
											Protocol::kMaxMessageLength);
		if (read_status < 1)
		{
			printf("Failed reading from server!\n");
		}

        Fputs(response, stdout);
        fflush(stdout);
	}
	return nullptr;
}

void key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mode*/)
{
    char request[Protocol::kMaxMessageLength];

    if(action == GLFW_PRESS)
    {
        switch(key)
        {
        case GLFW_KEY_ESCAPE:
            window->CloseWindow();
			game_running.store(false);
			strcpy(request, "QUIT\n");
            break;
		case GLFW_KEY_S:
			strcpy(request, "START\n");
			break;
		case GLFW_KEY_P:
			strcpy(request, "PAUSE\n");
			break;

		case GLFW_KEY_UP:
			strcpy(request, "MOVE 0.1 0.0\n");
			break;
		case GLFW_KEY_DOWN:
			strcpy(request, "MOVE -0.1 0.0\n");
			break;
		case GLFW_KEY_LEFT:
			strcpy(request, "MOVE 0.0 -0.1\n");
			break;
		case GLFW_KEY_RIGHT:
			strcpy(request, "MOVE 0.0 0.1\n");
			break;

        default:
            return;
        }

		ssize_t write_status = rio_writen(clientfd, request, strlen(request));
		if (write_status < 1)
		{
			printf("Failed writing to server!\n");
		}
    }
}

int main(int argc, char **argv)
{
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

	// Connect to server
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd);
	Rio_readinitb(&rio_read, clientfd);
	printf("Connected to server...\n");

	// Initialize client
	Rio_readinitb(&response_rio, clientfd);
	int mutex_init_status = pthread_mutex_init(&game_mutex, nullptr);
	assert(mutex_init_status == 0);
	game_running = true;

	void* thread_args = nullptr;
	Pthread_create(&server_response_tid, nullptr, ServerResponseThread, thread_args);

	// Initialize window and other graphics objects
	window = new Windows::WindowedWindow("Client", 200, Windows::ASPECT_RATIO_1_1);
	window->SetClearBits(GL_COLOR_BUFFER_BIT);
	window->SetClearColor(0.2f, 0.3f, 0.5f);
	window->SetKeyCallback(key_callback);

	// Game loop
	while (window->IsRunning())
	{
		window->PollEvents();
		window->ClearWindow();

		// render game

		window->SwapBuffers();
	}

	void* thread_return_status;
	Pthread_join(server_response_tid, &thread_return_status);

	delete window;
    Close(clientfd);
    exit(0);
}
