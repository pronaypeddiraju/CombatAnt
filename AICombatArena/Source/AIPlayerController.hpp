#pragma once
#include "ArenaPlayerInterface.hpp"
#include <mutex>

//------------------------------------------------------------------------------------------------------------------------------
class AIPlayerController
{
public:

	//Static singleton methods
	static AIPlayerController* GetInstance();
	static AIPlayerController* CreateInstance();
	static void DestroyInstance();

	void				Startup(const StartupInfo& info);
	void				Shutdown(const MatchResults& results);

	void				ThreadEntry(int threadIdx);

	void				ReceiveTurnState(const ArenaTurnStateForPlayer& state);
	bool				TurnOrderRequest(PlayerTurnOrders* orders);

private:
	void				ProcessTurn(ArenaTurnStateForPlayer& turnState);

	short				GetTileIndex(short x, short y) const;
	void				GetTileXYFromIndex(const short tileIndex, short &x, short&y);

	// Helpers
	void				MoveRandom(AgentReport currentAgent, int recursiveCount = 0);
	void				AddOrder(AgentID agent, eOrderCode order);
	void				ReturnClosestAmong(AgentReport currentAgent, short &returnX, short &returnY, short tile1X, short tile1Y, short tile2X, short tile2Y);
	bool				CheckTileSafetyForMove(AgentReport currentAgent, eOrderCode order);

	void				MoveToQueen(AgentReport currentAgent, int recursiveCount = 0);
	void				MoveToClosestFood(AgentReport currentAgent, int recursiveCount = 0);

	AgentReport			FindFirstAgentOfType(eAgentType type);
	eOrderCode			GetMoveOrderToTile(AgentReport currentAgent, short destPosX, short destPosY);
private:
	MatchInfo m_matchInfo;
	DebugInterface* m_debugInterface;

	int m_lastTurnProcessed;
	bool m_running;

	std::mutex m_turnLock;
	std::condition_variable m_turnCV;

	ArenaTurnStateForPlayer m_currentTurnInfo;
	PlayerTurnOrders m_turnOrders;

	AgentReport m_queenReport;

	int			m_numWorkers = 0;
};