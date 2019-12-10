#define ARENA_CLIENT
#include "ArenaPlayerInterface.hpp"
#include "AIPlayerController.hpp"
#include "RandomNumberGenerator.hpp"
#include "AICommons.hpp"
#include "ErrorWarningAssert.hpp"

volatile std::atomic<bool> gCanShutDown = false;
extern RandomNumberGenerator* g_RNG;

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
	return "Pronay Peddiraju";
}

//------------------------------------------------------------------------------------------------------------------------------
void PreGameStartup(const StartupInfo& info)
{
	g_RNG = new RandomNumberGenerator(0);

	AIPlayerController* player = AIPlayerController::CreateInstance();
	player->Startup(info);
}

//------------------------------------------------------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------------------------------------------------------
void PlayerThreadEntry(int yourThreadIdx)
{
	AIPlayerController* player = AIPlayerController::GetInstance();

	player->m_workerCostMapInitialized = false;
	player->m_scoutCostMapInitialized = false;
	player->m_soldierCostMapInitialized = false;

	player->m_agentIterator = 0;

	if (yourThreadIdx == 0)
	{
		player->WorkerThreadEntry(yourThreadIdx);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void ReceiveTurnState(const ArenaTurnStateForPlayer& state)
{
	AIPlayerController* player = AIPlayerController::GetInstance();
	player->ReceiveTurnState(state);
}

//------------------------------------------------------------------------------------------------------------------------------
bool TurnOrderRequest(int turnNumber, PlayerTurnOrders* ordersToFill)
{
	AIPlayerController* player = AIPlayerController::GetInstance();
	return player->TurnOrderRequest(ordersToFill);
}