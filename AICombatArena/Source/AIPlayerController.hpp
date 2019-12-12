#pragma once
#include "ArenaPlayerInterface.hpp"
#include "AStarPathing.hpp"
#include "Agent.hpp"
#include <mutex>
#include <atomic>
#include <map>

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

	void				SetVisionHeatMapForFood(std::vector<bool>& visionMap);
	void				AddOrder(AgentID agent, eOrderCode order);
	void				ReturnClosestAmong(Agent& currentAgent, short &returnX, short &returnY, short tile1X, short tile1Y, short tile2X, short tile2Y);
	bool				CheckTileSafetyForMove(Agent& currentAgent, eOrderCode order);
	eOrderCode			GetMoveOrderToTile(Agent& currentAgent, short destPosX, short destPosY);

	short				GetTileIndex(short x, short y) const;
	void				GetTileXYFromIndex(const short tileIndex, short &x, short&y);
	IntVec2				GetTileCoordinatesFromIndex(const short tileIndex);

	bool				IsAgentOnQueen(Agent& report);

private:
	void				ProcessTurn(ArenaTurnStateForPlayer& turnState);


	void				DebugDrawVisibleFood();
	void				UpdateAllAgentsFromTurnState(ArenaTurnStateForPlayer& turnState);
	void				CreateAgentFromReport(const AgentReport& agentReport);
	void				CheckAndAddAgentsToList(const AgentReport& agentReports);
	void				RemoveAnyDeadAgentsFromList();

	// Helpers
	void				MoveRandom(Agent& currentAgent, int recursiveCount = 0);
	

	void				MoveToQueen(Agent& currentAgent, int recursiveCount = 0);
	void				MoveToClosestFood(Agent& currentAgent, int recursiveCount = 0);
	void				PathToClosestFood(Agent& currentAgent);
	void				PathToClosestEnemy(Agent& currentAgent);
	void				PathToFarthestVisible(Agent& currentAgent);
	void				PathToQueen(Agent& currentAgent, bool shouldResetPath = false);
	void				PathToClosestDirt(Agent& currentAgent);

	IntVec2				GetFarthestObservedTile(const Agent& currentAgent);
	IntVec2				GetFarthestUnObservedTile(const Agent& currentAgent);

	AgentReport*		FindFirstAgentOfType(eAgentType type);
	int					FindClosestEnemy(Agent& currentAgent);
	IntVec2				FindClosestTile(Agent& currentAgent, eTileType tileType);
	eTileNeighborhood	IsPositionInNeighborhood(Agent& currentAgent, IntVec2 position);

	//Pathing
	int					GetTileCostForAgentType(eAgentType agentType, eTileType tileType);
	bool				IsTileSafeForAgentType(eTileType tileType, eAgentType agentType);
	bool				IsTileSafeForQueen(eTileType tileType);
	bool				IsTileSafeForScout(eTileType tileType);
	bool				IsTileSafeForSoldier(eTileType tileType);
	bool				IsTileSafeForWorker(eTileType tileType);

	bool				IsThisAgentQueen(Agent& report);
	int					GetClosestQueenTileIndex(Agent& report);
	int					IsEnemyInNeighborhood(int closestEnemy, Agent& report);

	bool				IsObservedAgentInAssignedTargets(ObservedAgent observedAgents);
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

	AStarPather m_pather;

	std::vector<Agent>	m_agentList;
	std::vector<ObservedAgent> m_assignedTargets;
	int lastAgent = 6;

	std::vector<int>	m_scoutDestinations;
	bool	m_repathOnQueenMove = false;

	int m_moveDelay = 100;

public:

	bool	m_workerCostMapInitialized = false;
	bool	m_soldierCostMapInitialized = false;
	bool	m_scoutCostMapInitialized = false;

	int		m_agentIterator = 0;

	std::vector<int>	m_costMapWorkers;
	std::vector<int>	m_costMapSoldiers;
	std::vector<int>	m_costMapScouts;

	std::vector<bool>	m_foodVisionHeatMap;

	std::map<int, Agent*> m_scoutPositionMap;

	int			m_numWorkers = 0;
	int			m_numSoldiers = 0;
	int			m_numScouts = 0;
	int			m_numQueens = 1;
};