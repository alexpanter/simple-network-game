#pragma once

#include <pthread.h>
#include <vector>

class Player
{
public:
	float posX;
	float posY;
	bool is_alive;
	bool is_ready;
	unsigned int player_id;

	static unsigned int id_generator;
	Player();
};


enum class GameStateType
{
	not_started,
	running,
	paused,
	ended
};


class Game
{
public:
	Game();
	~Game();
	bool MovePlayer(Player* player, float dirX, float dirY);

	// returns false if player is already ready
	bool PlayerSetReady(Player* player);

	// will set the game state to "running" and return `true`,
	// if all players are ready.
	bool TryStartGame();

	bool AddPlayer(Player* player);

	// will return false if player has already left the game
	bool PlayerQuit(Player* player);

	// returns true if game state is changed (ie. same player who
	// paused the game now unpauses it, or the other way around).
	bool PauseUnpauseGame(Player* player);

	GameStateType GetGameState();

private:
	struct ScopedLock
	{
		ScopedLock(pthread_mutex_t* m){ _m = m; pthread_mutex_lock(m); }
		~ScopedLock(){ pthread_mutex_unlock(_m); }
	private:
		pthread_mutex_t* _m;
	};
	pthread_mutex_t mGameMutex;

	std::vector<Player*> mPlayers;
	static constexpr int mkMaxPlayers = 4;

	GameStateType mGameState;

	unsigned int mPausedByPlayerId;
};
