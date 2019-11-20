#define ARENA_CLIENT
#include "ArenaPlayerInterface.hpp"
#include "AIPlayerController.hpp"
#include "RandomNumberGenerator.hpp"
#include "AICommons.hpp"
#include "ErrorWarningAssert.hpp"

volatile std::atomic<bool> gCanShutDown = false;
extern RandomNumberGenerator* g_RNG;

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
	g_RNG = new RandomNumberGenerator(0);

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
	TODO("Make this threaded and use a job system with behavior trees");
	AIPlayerController* player = AIPlayerController::GetInstance();
	if (yourThreadIdx == 0)
	{
		//Keep this to be the main thread
		//player->MainThreadEntry(yourThreadIdx);
		player->WorkerThreadEntry(yourThreadIdx);
		DebuggerPrintf("Test");
	}
	else
	{
		//player->WorkerThreadEntry(yourThreadIdx);
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