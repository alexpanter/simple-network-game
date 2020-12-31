#include "game.hpp"

#include <cstdio>
#include <cassert>


Game::Game()
{
	int mutex_status = pthread_mutex_init(&mGameMutex, nullptr);
	assert(mutex_status == 0);
	mGameState = GameStateType::not_started;
	mPausedByPlayerId = 0; // invalid player id
}

Game::~Game()
{
	pthread_mutex_destroy(&mGameMutex);
}


bool Game::MovePlayer(Player* player, float dirX, float dirY)
{
	ScopedLock lock(&mGameMutex);
	if (mGameState != GameStateType::running) return false;

	float newX = player->posX + dirX;
	float newY = player->posY + dirY;
	if (newX > 1.0f || newX < -1.0f || newY > 1.0f || newY < -1.0f) return false;

	player->posX = newX;
	player->posY = newY;
	return true;
}

bool Game::PlayerSetReady(Player* player)
{
	ScopedLock lock(&mGameMutex);
	if (mGameState != GameStateType::not_started) return false;
	if (player->is_ready) return false;

	player->is_ready = true;
	return true;
}

bool Game::AddPlayer(Player* player)
{
	ScopedLock lock(&mGameMutex);
	if (mGameState != GameStateType::not_started) return false;
	if (mPlayers.size() >= GameSettings::kMaxPlayers) return false;
	mPlayers.push_back(player);

	player->is_alive = true;
	player->is_ready = false;
	// TODO: Create proper player positions
	player->posX = 0.0f;
	player->posY = 0.0f;
	return true;
}

bool Game::TryStartGame()
{
	ScopedLock lock(&mGameMutex);
	if (mGameState == GameStateType::running) return false;

	bool may_start = true;
	for (auto& player : mPlayers)
	{
		may_start &= player->is_ready;
	}
	if (may_start) mGameState = GameStateType::running;
	return may_start;
}

bool Game::PlayerQuit(Player* player)
{
	ScopedLock lock(&mGameMutex);
	if (!player->is_alive) return false;

	player->is_alive = false;
	mGameState = GameStateType::ended;
	return true;
}

bool Game::PauseUnpauseGame(Player* player)
{
	ScopedLock lock(&mGameMutex);

	if (mGameState == GameStateType::running)
	{
		printf("Game running --> setting to pause!\n");
		mGameState = GameStateType::paused;
		mPausedByPlayerId = player->player_id;
		return true;
	}
	else if (mGameState == GameStateType::paused)
	{
		if (player->player_id != mPausedByPlayerId)
		{
			printf("Game paused --> cannot unpause by another player!\n");
			return false;
		}
		printf("Game paused --> setting to running!\n");

		mGameState = GameStateType::running;
		mPausedByPlayerId = 0;
		return true;
	}
	else
	{
		return false;
	}
}

GameStateType Game::GetGameState()
{
	ScopedLock lock(&mGameMutex);
	return mGameState;
}
