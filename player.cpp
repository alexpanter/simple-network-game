#include "player.hpp"
#include <cstdio>

PlayerId Player::id_generator = 1;


Player::Player()
{
	player_id = id_generator++;
	printf("new player with id=%u\n", player_id);
}
