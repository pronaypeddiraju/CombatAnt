#define ARENA_CLIENT
#include "ArenaPlayerInterface.hpp"
#include "AIPlayerController.hpp"
#include "AICommons.hpp"

volatile std::atomic<bool> gCanShutDown = false;

// info collection
//------------------------------------------------------------------------------------------------------------------------------
int GiveCommonInterfaceVersion()
{
	return COMMON_INTERFACE_VERSION_NUMBER;
}

//------------------------------------------------------------------------------------------------------------------------------
const char* GivePlayerName()
{
	return "United Ants of India";
}

//------------------------------------------------------------------------------------------------------------------------------
const char* GiveAuthorName()
{
	return "Pronay";
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

	while (gCanShutDown != true)
	{
		//Do nothing
		//The Ant arena (the ant arena thread will be here) needs to wait till the gCanShutDown is set as true by the threads from my DLL
	}

	delete player;
}

void PlayerThreadEntry(int yourThreadIdx)
{
	// this Ai is "bad" - I'm only going to use one worker thread
	TODO("Make this threaded and use a job system with behavior trees");
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