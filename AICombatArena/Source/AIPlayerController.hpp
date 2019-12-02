#pragma once
#include "ArenaPlayerInterface.hpp"
#include "AStarPathing.hpp"
#include <mutex>
#include <atomic>

enum eTileNeighborhood
{
	DIRECTION_EAST = 0,
	DIRECTION_WEST,
	DIRECTION_NORTH,
	DIRECTION_SOUTH,
	DIRECTION_NOT_NEIGHBOR,
	DIRECTION_THIS_TILE
};

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

	void				MainThreadEntry(int threadIdx);
	void				WorkerThreadEntry(int threadIdx);

	void				ReceiveTurnState(const ArenaTurnStateForPlayer& state);
	bool				TurnOrderRequest(PlayerTurnOrders* orders);

	void				SetMapCostBasedOnAntVision(eAgentType agentType, std::vector<int>& costMap);

private:
	void				ProcessTurn(ArenaTurnStateForPlayer& turnState);

	short				GetTileIndex(short x, short y) const;
	void				GetTileXYFromIndex(const short tileIndex, short &x, short&y);
	IntVec2				GetTileCoordinatesFromIndex(const short tileIndex);

	// Helpers
	void				MoveRandom(AgentReport& currentAgent, int recursiveCount = 0);
	void				AddOrder(AgentID agent, eOrderCode order);
	void				ReturnClosestAmong(AgentReport& currentAgent, short &returnX, short &returnY, short tile1X, short tile1Y, short tile2X, short tile2Y);
	bool				CheckTileSafetyForMove(AgentReport& currentAgent, eOrderCode order);

	void				MoveToQueen(AgentReport& currentAgent, int recursiveCount = 0);
	void				MoveToClosestFood(AgentReport& currentAgent, int recursiveCount = 0);
	void				PathToClosestFood(AgentReport& currentAgent);
	void				PathToClosestEnemy(AgentReport& currentAgent);
	void				PathToFarthestVisible(AgentReport& currentAgent);
	void				PathToQueen(AgentReport& currentAgent);
	void				PathToClosestDirt(AgentReport& currentAgent);

	IntVec2				GetFarthestObservedTile(const AgentReport& currentAgent);
	IntVec2				GetFarthestUnObservedTile(const AgentReport& currentAgent);

	AgentReport*		FindFirstAgentOfType(eAgentType type);
	int					FindClosestEnemy(AgentReport& currentAgent);
	IntVec2				FindClosestTile(AgentReport& currentAgent, eTileType tileType);
	eOrderCode			GetMoveOrderToTile(AgentReport& currentAgent, short destPosX, short destPosY);
	eTileNeighborhood	IsPositionInNeighborhood(AgentReport& currentAgent, IntVec2 position);

	//Pathing
	int					GetTileCostForAgentType(eAgentType agentType, eTileType tileType);
	bool				IsTileSafeForAgentType(eTileType tileType, eAgentType agentType);
	bool				IsTileSafeForQueen(eTileType tileType);
	bool				IsTileSafeForScout(eTileType tileType);
	bool				IsTileSafeForSoldier(eTileType tileType);
	bool				IsTileSafeForWorker(eTileType tileType);

	bool				IsThisAgentQueen(AgentReport& report);
	int					GetClosestQueenTileIndex(AgentReport& report);
	int					IsEnemyInNeighborhood(int closestEnemy, AgentReport& report);

private:
	MatchInfo m_matchInfo;
	DebugInterface* m_debugInterface;

	int m_lastTurnProcessed;
	volatile bool m_running;

	std::mutex m_turnLock;
	std::condition_variable m_turnCV;

	ArenaTurnStateForPlayer m_currentTurnInfo;
	PlayerTurnOrders m_turnOrders;

	std::vector<AgentReport> m_queenReports;
	AgentReport m_mainQueen;

	//Pather m_pather;
	//Pather m_scoutPather;
	//Pather m_soldierPather;
	//Pather m_workerPather;
	AStarPather m_pather;

public:

	std::atomic<bool>	m_workerCostMapInitialized = false;
	std::atomic<bool>	m_soldierCostMapInitialized = false;
	std::atomic<bool>	m_scoutCostMapInitialized = false;

	std::atomic<int>	m_agentIterator = 0;

	std::vector<int>	m_costMapWorkers;
	std::vector<int>	m_costMapSoldiers;
	std::vector<int>	m_costMapScouts;


	int			m_numWorkers = 0;
	int			m_numSoldiers = 0;
	int			m_numScouts = 0;
	int			m_numQueens = 1;
};