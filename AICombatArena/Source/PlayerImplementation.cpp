#define ARENA_CLIENT
#include "ArenaPlayerInterface.hpp"
#include "AIPlayerController.hpp"

// info collection
//------------------------------------------------------------------------------------------------------------------------------
int GiveCommonInterfaceVersion()
{
	return COMMON_INTERFACE_VERSION_NUMBER;
}

//------------------------------------------------------------------------------------------------------------------------------
const char* GivePlayerName()
{
	return "Chip Chapley";
}

//------------------------------------------------------------------------------------------------------------------------------
const char* GiveAuthorName()
{
	return "Chad Bradley";
}

// setup
void PreGameStartup(const StartupInfo& info)
{
	// make a player
	AIPlayerController* player = AIPlayerController::CreateInstance();
	player->Startup(info);
}

void PostGameShutdown(const MatchResults& results)
{
	AIPlayerController* player = AIPlayerController::GetInstance();
	player->Shutdown(results);
	delete player;
}

void PlayerThreadEntry(int yourThreadIdx)
{
	// this Ai is "bad" - I'm only going to use one worker thread
	if (yourThreadIdx == 0)
	{
		AIPlayerController* player = AIPlayerController::GetInstance();
		player->ThreadEntry(yourThreadIdx);
	}
}

// Turn
void ReceiveTurnState(const ArenaTurnStateForPlayer& state)
{
	AIPlayerController* player = AIPlayerController::GetInstance();
	player->ReceiveTurnState(state);
}

bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* ordersToFill)
{
	AIPlayerController* player = AIPlayerController::GetInstance();
	return player->TurnOrderRequest(ordersToFill);
}