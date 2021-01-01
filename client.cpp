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
#include <unordered_map>
#include <string>
#include "window.hpp"
#include "shaders.hpp"
#include "player.hpp"
#include "game_state.hpp"
#include "game_settings.hpp"

//
// GAME DATA
GLuint VAO, VBO;
GLuint next_VBO_index = 0;
struct ClientPlayerData
{
	Player player;
	GLuint VBO_index; // render index in vertex buffer
};
// vertex information will be
// (posX, posY, colorR, colorG, colorB)
const int kFloatsPerPlayer = 5;
GLfloat player_vertex_data[GameSettings::kMaxPlayers * kFloatsPerPlayer];
volatile std::atomic_bool should_buffer_data;

pthread_mutex_t game_mutex;
std::unordered_map<unsigned int, ClientPlayerData*> players;
PlayerId my_player_id = 0; // invalid player_id
GameStateType game_state;
std::atomic_uint32_t render_count;


//
// SERVER CONNECTION HANDLING
int clientfd;
rio_t response_rio;
pthread_t server_response_tid;
volatile std::atomic_bool game_running;


//
// DRAWING DATA
Windows::WindowedWindow* window;
const std::string window_main_title = "Client | ";
Shaders::ShaderWrapper* cubeShader;


void* ServerResponseThread(void* /*args*/)
{
	char response[Protocol::kMaxMessageLength];
	bool listen_to_server = true;
	printf("Starting server response thread\n");

	// intermediate variables for storing server response data
	PlayerId player_id = 0;
	float moveX, moveY;
	float colorR, colorG, colorB;

	while (game_running && listen_to_server)
	{
		memset(response, 0, Protocol::kMaxMessageLength);
		ssize_t read_status = Rio_readlineb(&response_rio, response,
											Protocol::kMaxMessageLength);
		if (read_status < 1)
		{
			printf("Failed reading from server!\n");
			continue;
		}
		printf("Server response: %s", response);

		if (strcmp(response, Protocol::SERVER_RESPONSE_START) == 0)
		{
			window->SetTitle(window_main_title + "game_running");
			game_state = GameStateType::running;
			render_count++;
		}
		else if (strcmp(response, Protocol::SERVER_RESPONSE_PAUSE) == 0)
		{
			window->SetTitle(window_main_title + "paused");
			game_state = GameStateType::paused;
		}
		else if (strcmp(response, Protocol::SERVER_RESPONSE_UNPAUSE) == 0)
		{
			window->SetTitle(window_main_title + "game_running");
			game_state = GameStateType::running;
		}
		else if (strcmp(response, Protocol::SERVER_RESPONSE_END_GAME) == 0)
		{
			window->SetTitle(window_main_title + "game over");
			game_state = GameStateType::ended;
			listen_to_server = false;
		}
		else if (sscanf(response, "SRV_RES_MOVE %u %f %f\n",
						&player_id, &moveX, &moveY) == 3)
		{
			printf("moving player...\n");
			pthread_mutex_lock(&game_mutex);

			auto it = players.find(player_id);
			if (it != players.end())
			{
				it->second->player.posX = moveX;
				it->second->player.posY = moveY;
				const int index = it->second->VBO_index * kFloatsPerPlayer;
				player_vertex_data[index] = moveX;
				player_vertex_data[index+1] = moveY;
				should_buffer_data.store(true);
				render_count++;
			}
			else
			{
				printf("ERROR: Cannot move - player_id %u is not in collection!\n",
					   player_id);
			}

			pthread_mutex_unlock(&game_mutex);
		}
		else if (sscanf(response, "SRV_RES_NEW_PLAYER %u %f %f\n",
						&player_id, &moveX, &moveY) == 3)
		{
			printf("Add another player with player_id=%u\n", player_id);
			pthread_mutex_lock(&game_mutex);

			auto it = players.find(player_id);
			if (it == players.end())
			{
				players[player_id] = new ClientPlayerData();
				ClientPlayerData* player = players[player_id];

				player->VBO_index = next_VBO_index;
				player->player.is_alive = true;
				player->player.posX = moveX;
				player->player.posY = moveY;
				player->player.player_id = player_id;

				const int index = player->VBO_index * kFloatsPerPlayer;
				player_vertex_data[index] = moveX;
				player_vertex_data[index+1] = moveY;
				player_vertex_data[index+2] = 0.8f;
				player_vertex_data[index+3] = 0.25f;
				player_vertex_data[index+4] = 0.3f;
				should_buffer_data.store(true);

				next_VBO_index++;
				render_count++;
			}
			else
			{
				printf("ERROR: Cannot add another new player - player_id %u already exists!\n",
					   player_id);
			}

			pthread_mutex_unlock(&game_mutex);
		}
		else if (sscanf(response, "SRV_RES_YOUR_NEW_PLAYER %u %f %f %f %f %f\n",
						&player_id, &moveX, &moveY,
						&colorR, &colorG, &colorB) == 6)
		{
			printf("Add my player with player_id=%u\n", player_id);
			pthread_mutex_lock(&game_mutex);

			auto it = players.find(player_id);
			if (it == players.end())
			{
				my_player_id = player_id;
				players[player_id] = new ClientPlayerData();
				ClientPlayerData* player = players[player_id];

				player->VBO_index = next_VBO_index;
				player->player.is_alive = true;
				player->player.posX = moveX;
				player->player.posY = moveY;
				player->player.colorR = colorR;
				player->player.colorG = colorG;
				player->player.colorB = colorB;
				player->player.player_id = player_id;

				const int index = player->VBO_index * kFloatsPerPlayer;
				player_vertex_data[index] = moveX;
				player_vertex_data[index+1] = moveY;
				player_vertex_data[index+2] = colorR;
				player_vertex_data[index+3] = colorG;
				player_vertex_data[index+4] = colorB;
				should_buffer_data.store(true);

				next_VBO_index++;
				render_count++;
			}
			else
			{
				printf("ERROR: Cannot add my new player - player_id %u already exists!\n",
					   player_id);
			}

			pthread_mutex_unlock(&game_mutex);
		}
	}

	printf("Terminating server response thread\n");
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
			window->SetTitle(window_main_title + "ready - waiting for other players..");
			strcpy(request, Protocol::CLIENT_REQUEST_START);
			break;
		case GLFW_KEY_P:
			strcpy(request, Protocol::CLIENT_REQUEST_TOGGLE_PAUSE);
			break;

		case GLFW_KEY_UP:
			Protocol::CreateMoveRequest(request, 0.0f, 0.1f);
			break;
		case GLFW_KEY_DOWN:
			Protocol::CreateMoveRequest(request, 0.0f, -0.1f);
			break;
		case GLFW_KEY_LEFT:
			Protocol::CreateMoveRequest(request, -0.1f, 0.0f);
			break;
		case GLFW_KEY_RIGHT:
			Protocol::CreateMoveRequest(request, 0.1f, 0.0f);
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


	// Initialize window
	std::string initial_title = window_main_title + "not_started";
	window = new Windows::WindowedWindow(initial_title.c_str(), 300,
										 Windows::ASPECT_RATIO_1_1);
	window->SetClearBits(GL_COLOR_BUFFER_BIT);
	window->SetClearColor(0.2f, 0.3f, 0.5f);
	window->SetKeyCallback(key_callback);


	// Initialize shader
	cubeShader = new Shaders::ShaderWrapper("shaders|cube_shader", Shaders::SHADERS_VGF);
	cubeShader->Activate();
	cubeShader->Deactivate();


	// Initialize player cube
	//player_vertex_data = new GLfloat[GameSettings::kMaxPlayers * kFloatsPerPlayer];

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// for (int i = 0; i < GameSettings::kMaxPlayers; i++)
	// {
	// 	player_vertex_data[i] = 0.0f;
	// }

    glBufferData(GL_ARRAY_BUFFER,
				 sizeof(GLfloat) * GameSettings::kMaxPlayers * kFloatsPerPlayer,
				 player_vertex_data, GL_DYNAMIC_DRAW);

    // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*kFloatsPerPlayer,
						  (GLvoid*)0);
    glEnableVertexAttribArray(0);
	// color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*kFloatsPerPlayer,
						  (GLvoid*)(sizeof(GLfloat)*2));
    glEnableVertexAttribArray(1);

	glBindVertexArray(0);


	// Initialize game state and server receive thread
	Rio_readinitb(&response_rio, clientfd);
	int mutex_init_status = pthread_mutex_init(&game_mutex, nullptr);
	assert(mutex_init_status == 0);
	game_running.store(true);
	game_state = GameStateType::not_started;
	render_count.store(1);
	should_buffer_data.store(false);

	void* thread_args = nullptr;
	Pthread_create(&server_response_tid, nullptr, ServerResponseThread, thread_args);


	// Game loop
	while (window->IsRunning())
	{

		window->PollEvents();

		if (render_count > 0)
		{
			window->ClearWindow();

			// check if vertex data was updated
			if (should_buffer_data)
			{
				should_buffer_data.store(false); // TODO: race condition, use counter!

				printf("Buffering data:\n");
				const int max_i = GameSettings::kMaxPlayers * kFloatsPerPlayer;
				for (int i = 0; i < max_i; i += 5)
				{
					printf("(%f, %f, %f, %f, %f)\n",
						   player_vertex_data[i],
						   player_vertex_data[i+1],
						   player_vertex_data[i+2],
						   player_vertex_data[i+3],
						   player_vertex_data[i+4]);
				}

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glFlush();

				// glBufferSubData(target buffer object type, offset in bytes,
				//                 size in bytes, pointer to data);
				glBufferSubData(GL_ARRAY_BUFFER, 0,
								sizeof(GLfloat) * GameSettings::kMaxPlayers * GameSettings::kMaxPlayers,
								player_vertex_data);

				// position
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
									  sizeof(GLfloat)*kFloatsPerPlayer,
									  (GLvoid*)0);
				glEnableVertexAttribArray(0);
				// color
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
									  sizeof(GLfloat)*kFloatsPerPlayer,
									  (GLvoid*)(sizeof(GLfloat)*2));
				glEnableVertexAttribArray(1);

				glBindVertexArray(0);
			}

			// render game
			cubeShader->Activate();
			glBindVertexArray(VAO);
			printf("will render %u points\n", next_VBO_index);
			glDrawArrays(GL_POINTS, 0, next_VBO_index);
			glBindVertexArray(0);
			cubeShader->Deactivate();

			window->SwapBuffers();

			render_count--;
		}
	}

	void* thread_return_status;
	Pthread_join(server_response_tid, &thread_return_status);

	glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
	//delete player_vertex_data;
	delete cubeShader;
	delete window;
    Close(clientfd);
    exit(0);
}
