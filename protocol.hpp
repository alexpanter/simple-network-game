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
#include "player.hpp"

namespace Protocol
{
	constexpr unsigned int kMaxMessageLength = 128;

	inline char CLIENT_REQUEST_START[] = "CLT_REQ_START\n";
	inline char CLIENT_REQUEST_TOGGLE_PAUSE[] = "CLT_REQ_TOGGLE_PAUSE\n";
	inline char CLIENT_REQUEST_QUIT[] = "CLT_REQ_QUIT\n";

	void CreateMoveRequest(char dest[kMaxMessageLength], float dirX, float dirY)
	{
		sprintf(dest, "CLT_REQ_MOVE %f %f\n", dirX, dirY);
	}


	inline char SERVER_RESPONSE_START[] = "SRV_RES_START\n";
	inline char SERVER_RESPONSE_PAUSE[] = "SRV_RES_PAUSE\n";
	inline char SERVER_RESPONSE_UNPAUSE[] = "SRV_RES_UNPAUSE\n";
	inline char SERVER_RESPONSE_END_GAME[] = "SRV_RES_END_GAME\n";

	void CreateMoveResponse(char dest[kMaxMessageLength], PlayerId player_id,
							float newposX, float newposY)
	{
		sprintf(dest, "SRV_RES_MOVE %u %f %f\n", player_id, newposX, newposY);
	}

	void CreateNewPlayerResponse(char dest[kMaxMessageLength],
								 PlayerId player_id, float posX, float posY)
	{
		sprintf(dest, "SRV_RES_NEW_PLAYER %u %f %f\n", player_id, posX, posY);
	}

	void CreateYourNewPlayerResponse(char dest[kMaxMessageLength],
									 PlayerId player_id, float posX, float posY,
									 float colorR, float colorG, float colorB)
	{
		sprintf(dest, "SRV_RES_YOUR_NEW_PLAYER %u %f %f %f %f %f\n",
				player_id, posX, posY, colorR, colorG, colorB);
	}
}
