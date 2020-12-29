/*
 * PROTOCOL DESIGN:
 *
 * "<message_type> <args...>"
 *
 * Message types:
 * MOVE, QUIT
 *
 * Examples:
 * "MOVE %f %f"	 ('%f' representing a float)
 */

#include <cstdio>

namespace Protocol
{
	constexpr unsigned int kMaxMessageLength = 128;

	enum class ClientRequestType
	{
		start,
		pause,
		quit,
		move,
	};

	enum class ServerResponseType
	{
		invalid_command,
		player_move,
		player_destroyed,
		player_win,
		game_start,
		game_pause,
		game_quit
	};

	struct ClientRequest
	{
		ClientRequestType Type;
		char* Content;
	};

	struct ServerResponse
	{
		ServerResponseType Type;
		char* Content;
	};
}
