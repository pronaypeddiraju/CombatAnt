#pragma once
#include "ArenaPlayerInterface.hpp"
#include <mutex>

//------------------------------------------------------------------------------------------------------------------------------
class AIPlayerController
{
public:

	static AIPlayerController* GetInstance();
	static AIPlayerController* CreateInstance();
	static void DestroyInstance();

	void Startup(const StartupInfo& info);
	void Shutdown(const MatchResults& results);

	void ThreadEntry(int threadIdx);

	void ReceiveTurnState(const ArenaTurnStateForPlayer& state);
	bool TurnOrderRequest(PlayerTurnOrders* orders);

public:
	void ProcessTurn(ArenaTurnStateForPlayer& turnState);

	short GetTileIndex(short x, short y) const;

	// Helpers
	void MoveRandom(AgentID agent);
	void AddOrder(AgentID agent, eOrderCode order);

public:
	MatchInfo m_matchInfo;
	DebugInterface* m_debugInterface;

	int m_lastTurnProcessed;
	bool m_running;

	std::mutex m_turnLock;
	std::condition_variable m_turnCV;

	ArenaTurnStateForPlayer m_currentTurnInfo;
	PlayerTurnOrders m_turnOrders;

};