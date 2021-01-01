#pragma once

typedef unsigned int PlayerId;

class Player
{
public:
	float posX;
	float posY;
	float colorR;
	float colorG;
	float colorB;

	bool is_alive;
	bool is_ready;
	PlayerId player_id;

	static PlayerId id_generator;
	Player();
};
