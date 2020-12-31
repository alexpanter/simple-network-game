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
#include "shaders.hpp"

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
			strcpy(request, Protocol::CLIENT_REQUEST_QUIT);
            break;
		case GLFW_KEY_S:
			strcpy(request, Protocol::CLIENT_REQUEST_START);
			break;
		case GLFW_KEY_P:
			strcpy(request, Protocol::CLIENT_REQUEST_TOGGLE_PAUSE);
			break;

		case GLFW_KEY_UP:
			Protocol::CreateMoveRequest(request, 0.1f, 0.0f);
			break;
		case GLFW_KEY_DOWN:
			Protocol::CreateMoveRequest(request, -0.1f, 0.0f);
			break;
		case GLFW_KEY_LEFT:
			Protocol::CreateMoveRequest(request, 0.0f, -0.1f);
			break;
		case GLFW_KEY_RIGHT:
			Protocol::CreateMoveRequest(request, 0.0f, 0.1f);
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

	// Initialize window
	char window_title[128];
	sprintf(window_title, "Client[connfd=%i]", clientfd);
	window = new Windows::WindowedWindow(window_title, 300, Windows::ASPECT_RATIO_1_1);
	window->SetClearBits(GL_COLOR_BUFFER_BIT);
	window->SetClearColor(0.2f, 0.3f, 0.5f);
	window->SetKeyCallback(key_callback);

	// Initialize shader
	Shaders::ShaderWrapper cubeShader("shaders|cube_shader", Shaders::SHADERS_VGF);
	cubeShader.Activate();
	glm::vec4 cubeData(0.0f, 0.0f, 0.2f, 0.2f);
	cubeShader.SetUniform("cubeData", const_cast<glm::vec4*>(&cubeData));
	cubeShader.Deactivate();

	// Initialize player cube
	GLfloat vertices[] = {
		0.0f, 0.0f
	};
	GLuint VBO, VAO;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);


	// Game loop
	while (window->IsRunning())
	{
		window->PollEvents();
		window->ClearWindow();

		// render game
		cubeShader.Activate();
		glBindVertexArray(VAO);
		glDrawArrays(GL_POINTS, 0, 6);
		cubeShader.Deactivate();

		window->SwapBuffers();
	}

	void* thread_return_status;
	Pthread_join(server_response_tid, &thread_return_status);

	glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
	delete window;
    Close(clientfd);
    exit(0);
}
