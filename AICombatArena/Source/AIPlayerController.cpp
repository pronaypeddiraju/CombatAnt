//------------------------------------------------------------------------------------------------------------------------------
#include "AIPlayerController.hpp"
#include "AICommons.hpp"
#include <math.h>

AIPlayerController* g_thePlayer = nullptr;
extern volatile std::atomic<bool> gCanShutDown;

//------------------------------------------------------------------------------------------------------------------------------
// Statics and locals
//------------------------------------------------------------------------------------------------------------------------------
static float RandomFloat01()
{
	return (float)rand() / (float)RAND_MAX;
}

//------------------------------------------------------------------------------------------------------------------------------
AIPlayerController* AIPlayerController::GetInstance()
{
	if (g_thePlayer == nullptr)
	{
		return CreateInstance();
	}

	return g_thePlayer;
}

//------------------------------------------------------------------------------------------------------------------------------
AIPlayerController* AIPlayerController::CreateInstance()
{
	if (g_thePlayer == nullptr)
	{
		g_thePlayer = new AIPlayerController();
	}
	
	return g_thePlayer;
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::DestroyInstance()
{
	if (g_thePlayer == nullptr)
	{
		return;
	}

	delete g_thePlayer;
	g_thePlayer = nullptr;
}

//------------------------------------------------------------------------------------------------------------------------------
// Class Methods
//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::Startup(const StartupInfo& info)
{
	m_matchInfo = info.matchInfo;
	m_debugInterface = info.debugInterface;

	// setup the turn number
	m_currentTurnInfo.turnNumber = -1;
	m_lastTurnProcessed = -1;
	m_running = true;
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::Shutdown(const MatchResults& results)
{
	m_running = false;
	m_turnCV.notify_all();
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::MainThreadEntry(int threadIdx)
{
	//Do main thread work here

	//Flood fill the map
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::WorkerThreadEntry(int threadIdx)
{
	// wait for data
	// process turn
	// mark turn as finished;
	ArenaTurnStateForPlayer turnState;

	IntVec2 mapSize = IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth);

	while (m_running)
	{
		std::unique_lock lk(m_turnLock);
		m_turnCV.wait(lk, [&]() { return !m_running || m_lastTurnProcessed != m_currentTurnInfo.turnNumber; });

		if (m_running)
		{
			turnState = m_currentTurnInfo;
			lk.unlock();

			m_pather.Init(mapSize, 1);
			//SetMapCostBasedOnAntVision(AGENT_TYPE_WORKER);
			//SetMapCostBasedOnAntVision(AGENT_TYPE_SCOUT);
			//SetMapCostBasedOnAntVision(AGENT_TYPE_SOLDIER);

			// process a turn and then mark that the turn is ready; 
			ProcessTurn(turnState);

			// notify the turn is ready; 
			m_lastTurnProcessed = turnState.turnNumber;
			m_debugInterface->LogText("Pronay's Turn Complete: %i", turnState.turnNumber);
		}
	}

	//This thread needs to now set the Shut Down condition as true(Not the ANT arena!)
	gCanShutDown = true;
	return;
}

//------------------------------------------------------------------------------------------------------------------------------
// This has to finish in less than 1MS otherwise you will be faulted
void AIPlayerController::ReceiveTurnState(const ArenaTurnStateForPlayer& state)
{
	// lock and copy information to thread
	{
		std::unique_lock lk(m_turnLock);
		m_currentTurnInfo = state;
	}

	// unlock, and notify
	m_turnCV.notify_one();
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::TurnOrderRequest(PlayerTurnOrders* orders)
{
	std::unique_lock lk(m_turnLock);
	if (m_lastTurnProcessed == m_currentTurnInfo.turnNumber)
	{
		*orders = m_turnOrders;
		return true;
	}
	else
	{
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::ProcessTurn(ArenaTurnStateForPlayer& turnState)
{
	// reset the orders
	m_turnOrders.numberOfOrders = 0;

	// for each ant I know about, give him something to do
	int agentCount = turnState.numReports;

	//Find the queen's location
	m_queenReport = FindFirstAgentOfType(AGENT_TYPE_QUEEN);

	// you will be given one report per agent you own (or did own the previous frame
	// in case of death).  For any live agent, you may make a command
	// this turn
	for (int i = 0; i < agentCount; ++i)
	{
		AgentReport& report = turnState.agentReports[i];
		if ((report.state != STATE_DEAD) && (report.exhaustion == 0))
		{
			// agent is alive and ready to get an order, so do something
			switch (report.type)
			{
				// scout will drunkely walk
			case AGENT_TYPE_SCOUT:
				MoveRandom(report);
				//PathToClosestFood(report);
				break;

				// moves randomly, but if they fall on food, will pick it up if hands are free
			case AGENT_TYPE_WORKER:
				if (report.state == STATE_HOLDING_FOOD)
				{
					if (report.tileX == m_queenReport.tileX && report.tileY == m_queenReport.tileY)
					{
						AddOrder(report.agentID, ORDER_DROP_CARRIED_OBJECT);
					}
					else
					{
						if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
						{
							//MoveRandom(report);
							PathToQueen(report);
						}
						else if (report.tileX == m_queenReport.tileX || report.tileY == m_queenReport.tileY)
						{
							//MoveToQueen(report);
							PathToQueen(report);
						}
						else
						{
							//MoveRandom(report);
							PathToQueen(report);
						}
					}
				}
				else
				{
					int tileIdx = GetTileIndex(report.tileX, report.tileY);
					bool tileHasFood = m_currentTurnInfo.tilesThatHaveFood[tileIdx];
					if (tileHasFood)
					{
						AddOrder(report.agentID, ORDER_PICK_UP_FOOD);
					}
					else
					{
						if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
						{
							MoveRandom(report);
						}
						else
						{
							//MoveToClosestFood(report);
							PathToClosestFood(report);
						}
					}
				}
				break;

				// Soldier randomly walks
			case AGENT_TYPE_SOLDIER:
				MoveRandom(report);
				break;

				// queen either moves or spawns randomly
			case AGENT_TYPE_QUEEN:
			{
				/*
				if (report.receivedCombatDamage > 0 || m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_SOLDIER)
				{
					if (m_numSoldiers < MAX_SOLDIERS)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_SOLDIER);
						AddOrder(report.agentID, order);
						m_numSoldiers++;
					}
				}
				else
				{
					
				}
				*/

				if (report.receivedCombatDamage > 0)
				{
					eOrderCode order = (eOrderCode)(ORDER_BIRTH_SOLDIER);
					AddOrder(report.agentID, order);
					m_numSoldiers++;
				}
				else
				{
					const float RANDOM_SPAWN_CHANCE = 0.5f;
					if (RandomFloat01() < RANDOM_SPAWN_CHANCE)
					{
						// spawn
						if (m_numWorkers < MAX_WORKERS && report.exhaustion == 0 && m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_WORKER)
						{
							int typeOffset = rand() % 3;
							eOrderCode order = (eOrderCode)(ORDER_BIRTH_WORKER);
							AddOrder(report.agentID, order);
							m_numWorkers++;
						}
					}
				}
			} break;
			}
		}
		else if (report.state == STATE_DEAD)
		{
			switch (report.type)
			{
			case AGENT_TYPE_SOLDIER:
				m_numSoldiers--;
				break;
			case AGENT_TYPE_WORKER:
				m_numWorkers--;
				break;
			}
		}
		else
		{
			//report.exhaustion == 0

		}

	}
}

//------------------------------------------------------------------------------------------------------------------------------
short AIPlayerController::GetTileIndex(short x, short y) const
{
	return y * m_matchInfo.mapWidth + x;
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::GetTileXYFromIndex(const short tileIndex, short &x, short&y)
{
	y = tileIndex / m_matchInfo.mapWidth;
	x = tileIndex % m_matchInfo.mapWidth;
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 AIPlayerController::GetTileCoordinatesFromIndex(const short tileIndex)
{
	IntVec2 tileCoords;
	tileCoords.x = tileIndex % m_matchInfo.mapWidth;
	tileCoords.y = tileIndex / m_matchInfo.mapWidth;
	return tileCoords;
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::MoveRandom(AgentReport currentAgent, int recursiveCount)
{
	int offset = rand() % 4;

	//eOrderCode randomDirection = PickRandomDirection();

	eOrderCode order = (eOrderCode)(ORDER_MOVE_EAST + offset);

	bool isSafe = CheckTileSafetyForMove(currentAgent, order);
	if (isSafe)
	{
		AddOrder(currentAgent.agentID, order);
	}
	else
	{
		if (recursiveCount < MAX_RECURSION_ALLOWED)
		{
			recursiveCount++;
			MoveRandom(currentAgent, recursiveCount);
		}
		else
		{
			AddOrder(currentAgent.agentID, order);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::AddOrder(AgentID agent, eOrderCode order)
{
	// TODO: Be sure that I don't double issue an order to an agent
	// Only first order will be processed by the server and a fault will be 
	// issued for any bad order
	// Ants not given ordres are assumed to idle

	// TODO: Make sure I'm not adding too many orders
	int agentIdx = m_turnOrders.numberOfOrders;

	m_turnOrders.orders[agentIdx].agentID = agent;
	m_turnOrders.orders[agentIdx].order = order;

	m_turnOrders.numberOfOrders++;
}

void AIPlayerController::ReturnClosestAmong(AgentReport currentAgent, short &returnX, short &returnY, short tile1X, short tile1Y, short tile2X, short tile2Y)
{
	//Find the closest manhattan distance among the 2 tiles
	short distanceXTile1 = abs(currentAgent.tileX - tile1X);
	short distanceYTile1 = abs(currentAgent.tileY - tile1Y);

	short distanceXTile2 = abs(currentAgent.tileX - tile2X);
	short distanceYTile2 = abs(currentAgent.tileY - tile2Y);

	short manhattanTile1 = distanceXTile1 + distanceYTile1;
	short manhattanTile2 = distanceXTile2 + distanceYTile2;

	if (manhattanTile1 < manhattanTile2)
	{
		returnX = tile1X;
		returnY = tile1Y;
	}
	else
	{
		returnX = tile2X;
		returnY = tile2Y;
	}

	return;
}

bool AIPlayerController::CheckTileSafetyForMove(AgentReport currentAgent, eOrderCode order)
{
	int index;

	switch (order)
	{
	case ORDER_HOLD:
		return true;

	case ORDER_MOVE_EAST:
		index = GetTileIndex(currentAgent.tileX + 1, currentAgent.tileY);
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_STONE)
		{
			return false;
		}
		else
		{
			return true;
		}
		
	case ORDER_MOVE_NORTH:
		index = GetTileIndex(currentAgent.tileX, currentAgent.tileY + 1);
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER)
		{
			return false;
		}
		else
		{
			return true;
		}
		
	case ORDER_MOVE_WEST:
		index = GetTileIndex(currentAgent.tileX - 1, currentAgent.tileY);
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_STONE)
		{
			return false;
		}
		else
		{
			return true;
		}
		
	case ORDER_MOVE_SOUTH:
		index = GetTileIndex(currentAgent.tileX, currentAgent.tileY - 1);
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER)
		{
			return false;
		}
		else
		{
			return true;
		}
		
	default:
		return true;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::MoveToQueen(AgentReport currentAgent, int recursiveCount)
{
	//Path towards that general direction
	eOrderCode moveOrder = GetMoveOrderToTile(currentAgent, m_queenReport.tileX, m_queenReport.tileY);

	bool isSafe = CheckTileSafetyForMove(currentAgent, moveOrder);
	if (isSafe)
	{
		AddOrder(currentAgent.agentID, moveOrder);
	}
	else
	{
		if (recursiveCount < MAX_RECURSION_ALLOWED)
		{
			recursiveCount++;
			MoveToQueen(currentAgent, recursiveCount);
		}
		else
		{
			AddOrder(currentAgent.agentID, moveOrder);
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::MoveToClosestFood(AgentReport currentAgent, int recursiveCount)
{
	short destX = 100;
	short destY = 100;
	int closestIndex = 0;

	//Look for visible food
	for (int hasFoodTileIndex = 0; hasFoodTileIndex < MAX_ARENA_TILES; hasFoodTileIndex++)
	{
		if (m_currentTurnInfo.tilesThatHaveFood[hasFoodTileIndex])
		{
			short foodFoundX = 0;
			short foodFoundY = 0;

			GetTileXYFromIndex(hasFoodTileIndex, foodFoundX, foodFoundY);

			short closestX = 0;
			short closestY = 0;

			ReturnClosestAmong(currentAgent, closestX, closestY, destX, destY, foodFoundX, foodFoundY);
			if (closestX == destX && closestY == destY)
			{
				closestIndex = hasFoodTileIndex;
			}

			destX = closestX;
			destY = closestY;
		}
	}

	//Move towards the closest visible food
	if (currentAgent.tileX == destX || currentAgent.tileY == destY)
	{
		eOrderCode order = GetMoveOrderToTile(currentAgent, destX, destY);

		bool isSafe = CheckTileSafetyForMove(currentAgent, order);
		if (isSafe)
		{
			AddOrder(currentAgent.agentID, order);
		}
		else
		{
			if (recursiveCount < MAX_RECURSION_ALLOWED)
			{
				recursiveCount++;
				m_currentTurnInfo.tilesThatHaveFood[closestIndex] = false;
				MoveToClosestFood(currentAgent, recursiveCount);
			}
			else
			{
				AddOrder(currentAgent.agentID, order);
			}
		}
	}
	else
	{
		MoveRandom(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToClosestFood(AgentReport currentAgent)
{
	short destX = 100;
	short destY = 100;
	int closestIndex = 0;

	//Look for visible food
	for (int hasFoodTileIndex = 0; hasFoodTileIndex < MAX_ARENA_TILES; hasFoodTileIndex++)
	{
		if (m_currentTurnInfo.tilesThatHaveFood[hasFoodTileIndex])
		{
			short foodFoundX = 0;
			short foodFoundY = 0;

			GetTileXYFromIndex(hasFoodTileIndex, foodFoundX, foodFoundY);

			short closestX = 0;
			short closestY = 0;

			ReturnClosestAmong(currentAgent, closestX, closestY, destX, destY, foodFoundX, foodFoundY);
			if (closestX == destX && closestY == destY)
			{
				closestIndex = hasFoodTileIndex;
			}

			destX = closestX;
			destY = closestY;
		}
	}

	//Now path to the closest food
	m_pathSolver = new PathSolver;
	Path path;
	path.clear();
	m_pathSolver->AddStart(IntVec2(currentAgent.tileX, currentAgent.tileY));
	m_pathSolver->AddEnd(IntVec2(destX, destY));
	m_pathSolver->StartDistanceField(&m_pather, &path);
	eOrderCode order = GetMoveOrderToTile(currentAgent, path[1].x, path[1].y);
	AddOrder(currentAgent.agentID, order);
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToQueen(AgentReport currentAgent)
{
	//Now path to the closest food
	m_pathSolver = new PathSolver;
	Path path;
	path.clear();
	m_pathSolver->AddStart(IntVec2(currentAgent.tileX, currentAgent.tileY));
	m_pathSolver->AddEnd(IntVec2(m_queenReport.tileX, m_queenReport.tileY));
	m_pathSolver->StartDistanceField(&m_pather, &path);
	eOrderCode order = GetMoveOrderToTile(currentAgent, path[1].x, path[1].y);
	AddOrder(currentAgent.agentID, order);
}

//------------------------------------------------------------------------------------------------------------------------------
AgentReport AIPlayerController::FindFirstAgentOfType(eAgentType type)
{
	//Loop through agents and return the first agent of eAgentType type
	int agentCount = m_currentTurnInfo.numReports;

	for (int agentIndex = 0; agentIndex < agentCount; agentIndex++)
	{
		if (m_currentTurnInfo.agentReports[agentIndex].type == type)
		{
			m_queenReport = m_currentTurnInfo.agentReports[agentIndex];
			break;
		}
	}

	return m_queenReport;
}

//------------------------------------------------------------------------------------------------------------------------------
eOrderCode AIPlayerController::GetMoveOrderToTile(AgentReport currentAgent, short destPosX, short destPosY)
{
	//Return the ordercode for move based on currentAgent position relative to destPosX and destPosY

	if (destPosY > currentAgent.tileY)
	{
		//Move up
		return ORDER_MOVE_NORTH;
	}
	else if (destPosY < currentAgent.tileY)
	{
		//Move down
		return ORDER_MOVE_SOUTH;
	}
	else if (destPosX < currentAgent.tileX)
	{
		//Move left
		return ORDER_MOVE_WEST;
	}
	else if (destPosX > currentAgent.tileX)
	{
		//Move right
		return ORDER_MOVE_EAST;
	}
	else
	{
		//Dont move
		return ORDER_HOLD;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::SetMapCostBasedOnAntVision(eAgentType agentType)
{
	//Check what is visible in the map and set costs accordingly
	for (int observedTileIndex = 0; observedTileIndex < m_matchInfo.mapWidth * m_matchInfo.mapWidth; observedTileIndex++)
	{
		int cost = GetTileCostForAgentType(agentType, m_currentTurnInfo.observedTiles[observedTileIndex]);
		IntVec2 coords = GetTileCoordinatesFromIndex(observedTileIndex);
		m_pather.SetCost(coords, (float)cost);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
int AIPlayerController::GetTileCostForAgentType(eAgentType agentType, eTileType tileType)
{
	switch (agentType)
	{
	case AGENT_TYPE_WORKER:
	{
		switch (tileType)
		{
		case TILE_TYPE_DIRT:
		case TILE_TYPE_AIR:
		case TILE_TYPE_CORPSE_BRIDGE:
			return 1;
			break;
		case TILE_TYPE_WATER:
			return 9999;
			break;
		case TILE_TYPE_UNSEEN:
			return 50;
			break;
		case 	TILE_TYPE_STONE:
			return 9999;
			break;
		}
	}
	break;
	case AGENT_TYPE_SCOUT:
	{
		switch (tileType)
		{
		case TILE_TYPE_DIRT:
		case TILE_TYPE_AIR:
		case TILE_TYPE_CORPSE_BRIDGE:
			return 1;
			break;
		case TILE_TYPE_WATER:
			return 9999;
			break;
		case TILE_TYPE_UNSEEN:
			return 1;
			break;
		case 	TILE_TYPE_STONE:
			return 9999;
			break;
		}
	}
		break;
	case AGENT_TYPE_SOLDIER:
	{
		switch (tileType)
		{
		case TILE_TYPE_DIRT:
			return 50;
			break;
		case TILE_TYPE_AIR:
		case TILE_TYPE_CORPSE_BRIDGE:
			return 1;
			break;
		case TILE_TYPE_WATER:
			return 9999;
			break;
		case TILE_TYPE_UNSEEN:
			return 1;
			break;
		case 	TILE_TYPE_STONE:
			return 9999;
			break;
		}
	}
		break;
	}
}
