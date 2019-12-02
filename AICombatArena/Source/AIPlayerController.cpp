//------------------------------------------------------------------------------------------------------------------------------
#include "AIPlayerController.hpp"
#include "AICommons.hpp"
#include "MathUtils.hpp"
#include <math.h>
#include "ErrorWarningAssert.hpp"

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

	m_queenReports = std::vector<AgentReport>(MAX_QUEENS);
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::Shutdown(const MatchResults& results)
{
	m_running = false;
	m_turnCV.notify_all();

	DebuggerPrintf("\n Largest Open List: %d", m_pather.m_largestOpenList);
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

			/*
			if (!m_workerCostMapInitialized)
			{
				m_workerCostMapInitialized = true;
				//SetMapCostBasedOnAntVision(AGENT_TYPE_WORKER, m_costMapWorkers);
			}

			if (!m_scoutCostMapInitialized)
			{
				m_scoutCostMapInitialized = true;
				//SetMapCostBasedOnAntVision(AGENT_TYPE_SCOUT, m_costMapScouts);
			}

			if (!m_soldierCostMapInitialized)
			{
				m_soldierCostMapInitialized = true;
				//SetMapCostBasedOnAntVision(AGENT_TYPE_SOLDIER, m_costMapSoldiers);
			}
			*/

			SetMapCostBasedOnAntVision(AGENT_TYPE_WORKER, m_costMapWorkers);
			SetMapCostBasedOnAntVision(AGENT_TYPE_SCOUT, m_costMapScouts);
			SetMapCostBasedOnAntVision(AGENT_TYPE_SOLDIER, m_costMapSoldiers);

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
	m_queenReports[0] = *FindFirstAgentOfType(AGENT_TYPE_QUEEN);

	// you will be given one report per agent you own (or did own the previous frame
	// in case of death).  For any live agent, you may make a command
	// this turn
	
	if (m_matchInfo.numTurnsBeforeSuddenDeath > turnState.turnNumber)
	{
		//while(m_agentIterator < agentCount)
		for (int i = 0; i < agentCount; ++i)
		{
			//If using while loop with atomic agent count iterator
			//AgentReport& report = turnState.agentReports[m_agentIterator];

			AgentReport& report = turnState.agentReports[i];
			if ((report.state != STATE_DEAD) && (report.exhaustion == 0))
			{
				// agent is alive and ready to get an order, so do something
				switch (report.type)
				{
					// scout will drunkely walk
				case AGENT_TYPE_SCOUT:
					//MoveRandom(report);
					PathToFarthestVisible(report);
					break;

					// moves randomly, but if they fall on food, will pick it up if hands are free
				case AGENT_TYPE_WORKER:
					if (report.state == STATE_HOLDING_FOOD)
					{
						if (report.tileX == m_queenReports[0].tileX && report.tileY == m_queenReports[0].tileY)
						{
							AddOrder(report.agentID, ORDER_DROP_CARRIED_OBJECT);
						}
						else
						{
							if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
							{
								MoveRandom(report);
							}
							else
							{
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
							PathToClosestFood(report);
						}
					}
					break;

				case AGENT_TYPE_SOLDIER:
				{
					if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
					{
						MoveRandom(report);
					}
					else
					{
						PathToClosestEnemy(report);
					}
				}
				break;

				// queen either moves or spawns randomly
				case AGENT_TYPE_QUEEN:
				{
					bool reportFound = false;
					for (int i = 0; i < m_numQueens; i++)
					{
						if (m_queenReports[i].agentID == report.agentID)
						{
							reportFound = true;
						}
					}

					if (!reportFound)
					{
						m_queenReports.push_back(report);
					}

					if (report.receivedCombatDamage > 0)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_SOLDIER);
						AddOrder(report.agentID, order);
						m_numSoldiers++;
					}
					else if (m_numSoldiers < MAX_SOLDIERS && m_numWorkers == MAX_WORKERS * m_numQueens&& m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_SOLDIER)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_SOLDIER);
						AddOrder(report.agentID, order);
						m_numSoldiers++;
					}
					else if (m_numScouts < MAX_SCOUTS && m_numSoldiers == MAX_SOLDIERS && m_numWorkers == MAX_WORKERS * m_numQueens && m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_SCOUT)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_SCOUT);
						AddOrder(report.agentID, order);
						m_numScouts++;
					}
					else if (m_numQueens < MAX_QUEENS && m_numScouts == MAX_SCOUTS && m_numSoldiers == MAX_SOLDIERS && m_numWorkers == MAX_WORKERS * m_numQueens && m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_QUEEN * m_numQueens)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_QUEEN);
						AddOrder(report.agentID, order);
						m_numQueens++;
					}
					else if (m_numWorkers < MAX_WORKERS * m_numQueens && report.exhaustion == 0 && m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN_WORKER)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_WORKER);
						AddOrder(report.agentID, order);
						m_numWorkers++;
					}
					else
					{
						/*
						if (report.agentID != m_mainQueen.agentID)
						{
							MoveRandom(report);
						}
						*/
					}
				}
				break;
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
				case AGENT_TYPE_SCOUT:
					m_numScouts--;
					break;
				case AGENT_TYPE_QUEEN:
					m_numQueens--;
					break;
				}
			}
			//When using while loop instead of for, uncomment this
			//++m_agentIterator;

		}
	}
	else
	{
		//while(m_agentIterator < agentCount)
		for (int i = 0; i < agentCount; ++i)
		{
			//If using while loop with atomic agent count iterator
			//AgentReport& report = turnState.agentReports[m_agentIterator];

			AgentReport& report = turnState.agentReports[i];
			if (report.state != STATE_DEAD)
			{
				switch (report.type)
				{
				case AGENT_TYPE_SOLDIER:
				{
					if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
					{
						MoveRandom(report);
					}
					else
					{
						PathToClosestEnemy(report);
					}
					break;
				}
				case AGENT_TYPE_SCOUT:
				{
					eOrderCode order = (eOrderCode)(ORDER_SUICIDE);
					AddOrder(report.agentID, order);
					m_numScouts--;
					/*
					if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
					{
						MoveRandom(report);
					}
					else
					{
						PathToFarthestVisible(report);
					}
					*/
					break;
				}
				case AGENT_TYPE_WORKER:
				{
					
					if (report.state == STATE_HOLDING_FOOD)
					{
						if (report.tileX == m_queenReports[0].tileX && report.tileY == m_queenReports[0].tileY)
						{
							AddOrder(report.agentID, ORDER_DROP_CARRIED_OBJECT);
						}
						else
						{
							if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
							{
								MoveRandom(report);
							}
							else
							{
								PathToQueen(report);
							}
						}
					}
					else
					{
						eOrderCode order = (eOrderCode)(ORDER_SUICIDE);
						AddOrder(report.agentID, order);
						m_numWorkers--;

						/*
						int tileIdx = GetTileIndex(report.tileX, report.tileY);
						bool tileHasFood = m_currentTurnInfo.tilesThatHaveFood[tileIdx];
						if (tileHasFood)
						{
							AddOrder(report.agentID, ORDER_PICK_UP_FOOD);
						}
						else
						{
							PathToClosestFood(report);
						}
						*/
					}
					
					/*
					if (report.state == STATE_HOLDING_DIRT)
					{
						int closestEnemy = FindClosestEnemy(report);
						if (turnState.observedAgents[closestEnemy].tileX == report.tileX && turnState.observedAgents[closestEnemy].tileY == report.tileY)
						{
							eOrderCode order = (eOrderCode)(ORDER_DROP_CARRIED_OBJECT);
							AddOrder(report.agentID, order);
						}
						else
						{
							if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
							{
								MoveRandom(report);
							}
							else
							{
								PathToClosestEnemy(report);
							}
						}
					}
					else
					{
						IntVec2 closestDirt = FindClosestTile(report, TILE_TYPE_DIRT);
						eTileNeighborhood dirtNeighborhood = IsPositionInNeighborhood(report, closestDirt);
						switch (dirtNeighborhood)
						{
						case DIRECTION_NORTH:
						{
							eOrderCode order = (eOrderCode)(ORDER_DIG_NORTH);
							AddOrder(report.agentID, order);
						}
						break;
						case DIRECTION_SOUTH:
						{
							eOrderCode order = (eOrderCode)(ORDER_DIG_SOUTH);
							AddOrder(report.agentID, order);
						}
						break;
						case DIRECTION_EAST:
						{
							eOrderCode order = (eOrderCode)(ORDER_DIG_EAST);
							AddOrder(report.agentID, order);
						}
						break;
						case DIRECTION_WEST:
						{
							eOrderCode order = (eOrderCode)(ORDER_DIG_WEST);
							AddOrder(report.agentID, order);
						}
						break;
						case DIRECTION_THIS_TILE:
						{
							eOrderCode order = (eOrderCode)(ORDER_DIG_HERE);
							AddOrder(report.agentID, order);
						}
						case DIRECTION_NOT_NEIGHBOR:
						{
							if (report.result == AGENT_ORDER_ERROR_MOVE_BLOCKED_BY_TILE)
							{
								MoveRandom(report);
							}
							else
							{
								PathToClosestDirt(report);
							}
						}
						break;
						}
					}
				*/

				break;
				}
				case AGENT_TYPE_QUEEN:
				{
					if (report.receivedCombatDamage > 0)
					{
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_SOLDIER);
						AddOrder(report.agentID, order);
						m_numSoldiers++;
					}

					/*
					if (m_queenReports.size() > 1)
					{
						for (int i = 1; i < m_queenReports.size(); i++)
						{
							if (report.agentID == m_queenReports[i].agentID)
							{
								eOrderCode order = (eOrderCode)(ORDER_SUICIDE);
								AddOrder(report.agentID, order);
								m_numQueens--;
								m_queenReports.erase(m_queenReports.begin() + i);
							}
						}

						return;
					}
					*/

					break;
				}
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
				case AGENT_TYPE_SCOUT:
					m_numScouts--;
					break;
				case AGENT_TYPE_QUEEN:
					m_numQueens--;
					break;
				}
			}
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
void AIPlayerController::MoveRandom(AgentReport& currentAgent, int recursiveCount)
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

void AIPlayerController::ReturnClosestAmong(AgentReport& currentAgent, short &returnX, short &returnY, short tile1X, short tile1Y, short tile2X, short tile2Y)
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

bool AIPlayerController::CheckTileSafetyForMove(AgentReport& currentAgent, eOrderCode order)
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
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_STONE)
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
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_STONE)
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
void AIPlayerController::MoveToQueen(AgentReport& currentAgent, int recursiveCount)
{
	//Path towards that general direction
	int tileIndex = GetClosestQueenTileIndex(currentAgent);
	IntVec2 tileCoords = GetTileCoordinatesFromIndex(tileIndex);
	eOrderCode moveOrder = GetMoveOrderToTile(currentAgent, tileCoords.x, tileCoords.y);

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
void AIPlayerController::MoveToClosestFood(AgentReport& currentAgent, int recursiveCount)
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
void AIPlayerController::PathToClosestFood(AgentReport& currentAgent)
{
	short destX = 9999;
	short destY = 9999;
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
	Path path;
	path.clear();

	int startIndex = GetTileIndex(currentAgent.tileX, currentAgent.tileY);
	int endIndex = GetTileIndex(destX, destY);

	if (destX != 9999)
	{
		//m_currentTurnInfo.tilesThatHaveFood[endIndex] = false;

		path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapWorkers);
		eOrderCode order = GetMoveOrderToTile(currentAgent, path.back().x, path.back().y);
		AddOrder(currentAgent.agentID, order);
	}
	else
	{
		MoveRandom(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToClosestEnemy(AgentReport& currentAgent)
{
	int observedAgentID = FindClosestEnemy(currentAgent);

	if (observedAgentID != -1)
	{
		ObservedAgent target = m_currentTurnInfo.observedAgents[observedAgentID];

		Path path;
		path.clear();

		int startIndex = GetTileIndex(currentAgent.tileX, currentAgent.tileY);
		int endIndex = GetTileIndex(target.tileX, target.tileY);

		if (currentAgent.type == AGENT_TYPE_SOLDIER)
		{
			path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapSoldiers, m_matchInfo.mapWidth * 2);
		}
		else if (currentAgent.type == AGENT_TYPE_WORKER)
		{
			path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapWorkers, m_matchInfo.mapWidth * 2);
		}
		
		if (path.size() != 0)
		{
			eOrderCode order = GetMoveOrderToTile(currentAgent, path.back().x, path.back().y);

			AddOrder(currentAgent.agentID, order);
		}
		else
		{
			MoveRandom(currentAgent);
		}
	}
	else
	{
		PathToQueen(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToFarthestVisible(AgentReport& currentAgent)
{
	//Find the farthest observable tile
	IntVec2 farthestTile = GetFarthestObservedTile(currentAgent);

	Path path;
	path.clear();

	int startIndex = GetTileIndex(currentAgent.tileX, currentAgent.tileY);
	int endIndex = GetTileIndex(farthestTile.x, farthestTile.y);

	if (path.size() != 0)
	{
		path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapScouts, 100);
		
		eOrderCode order = GetMoveOrderToTile(currentAgent, path.back().x, path.back().y);
		AddOrder(currentAgent.agentID, order);
	}
	else
	{
		MoveRandom(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToQueen(AgentReport& currentAgent)
{
	//Now path to the closest food
	Path path;
	path.clear();

	int startIndex = GetTileIndex(currentAgent.tileX, currentAgent.tileY);
	int endIndex = GetClosestQueenTileIndex(currentAgent);
	//int endIndex = GetTileIndex(m_queenReport.tileX, m_queenReport.tileY);

	if (currentAgent.type == AGENT_TYPE_WORKER)
	{
		path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapWorkers);
	}
	else if (currentAgent.type == AGENT_TYPE_SOLDIER)
	{
		path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapSoldiers);
	}

	if (path.size() != 0)
	{
		eOrderCode order = GetMoveOrderToTile(currentAgent, path.back().x, path.back().y);
		AddOrder(currentAgent.agentID, order);
	}
	else
	{
		MoveRandom(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::PathToClosestDirt(AgentReport& currentAgent)
{
	short destX = 9999;
	short destY = 9999;
	int closestIndex = 0;

	//Look for visible tiles
	for (int dirtIndex = 0; dirtIndex < MAX_ARENA_TILES; dirtIndex++)
	{
		if (m_currentTurnInfo.observedTiles[dirtIndex])
		{
			short dirtFoundX = 0;
			short dirtFoundY = 0;

			GetTileXYFromIndex(dirtIndex, dirtFoundX, dirtFoundY);

			short closestX = 0;
			short closestY = 0;

			ReturnClosestAmong(currentAgent, closestX, closestY, destX, destY, dirtFoundX, dirtFoundY);
			if (closestX == destX && closestY == destY)
			{
				closestIndex = dirtIndex;
			}

			destX = closestX;
			destY = closestY;
		}
	}

	//Now path to the closest food
	Path path;
	path.clear();

	int startIndex = GetTileIndex(currentAgent.tileX, currentAgent.tileY);
	int endIndex = GetTileIndex(destX, destY);

	if (destX != 9999)
	{
		path = m_pather.CreatePathAStar(startIndex, endIndex, IntVec2(m_matchInfo.mapWidth, m_matchInfo.mapWidth), m_costMapWorkers);
		eOrderCode order = GetMoveOrderToTile(currentAgent, path.back().x, path.back().y);
		AddOrder(currentAgent.agentID, order);
	}
	else
	{
		MoveRandom(currentAgent);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 AIPlayerController::GetFarthestObservedTile(const AgentReport& currentAgent)
{
	int maxDistance = 0;
	int farthestIndex = -1;
	for (int tileIndex = 0; tileIndex < m_matchInfo.mapWidth * m_matchInfo.mapWidth; tileIndex++)
	{
		if (m_currentTurnInfo.observedTiles[tileIndex] != TILE_TYPE_UNSEEN && m_currentTurnInfo.observedTiles[tileIndex] != TILE_TYPE_WATER && m_currentTurnInfo.observedTiles[tileIndex] != TILE_TYPE_STONE)
		{
			int distance = GetManhattanDistance(IntVec2(currentAgent.tileX, currentAgent.tileY), GetTileCoordinatesFromIndex(tileIndex));
			if (distance > maxDistance)
			{
				farthestIndex = tileIndex;
				maxDistance = distance;
			}
		}
	}

	return GetTileCoordinatesFromIndex(farthestIndex);
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 AIPlayerController::GetFarthestUnObservedTile(const AgentReport& currentAgent)
{
	int maxDistance = 0;
	int farthestIndex = -1;
	for (int tileIndex = 0; tileIndex < m_matchInfo.mapWidth * m_matchInfo.mapWidth; tileIndex++)
	{
		if (m_currentTurnInfo.observedTiles[tileIndex] == TILE_TYPE_UNSEEN)
		{
			int distance = GetManhattanDistance(IntVec2(currentAgent.tileX, currentAgent.tileY), GetTileCoordinatesFromIndex(tileIndex));
			if (distance > maxDistance)
			{
				farthestIndex = tileIndex;
			}
		}
	}

	return GetTileCoordinatesFromIndex(farthestIndex);
}

//------------------------------------------------------------------------------------------------------------------------------
AgentReport* AIPlayerController::FindFirstAgentOfType(eAgentType type)
{
	//Loop through agents and return the first agent of eAgentType type
	int agentCount = m_currentTurnInfo.numReports;
	AgentReport* report = nullptr;

	for (int agentIndex = 0; agentIndex < agentCount; agentIndex++)
	{
		if (m_currentTurnInfo.agentReports[agentIndex].type == type)
		{
			report = &m_currentTurnInfo.agentReports[agentIndex];
			if (m_mainQueen.agentID == 0)
			{
				m_mainQueen = m_currentTurnInfo.agentReports[agentIndex];
			}
			break;
		}
	}

	return report;
}

//------------------------------------------------------------------------------------------------------------------------------
int AIPlayerController::FindClosestEnemy(AgentReport& currentAgent)
{
	int closestDistance = 999999;
	int closestIndex = -1;

	for (int observeIndex = 0; observeIndex < m_currentTurnInfo.numObservedAgents; observeIndex++)
	{
		//Find the closest among these guys
		int distance = GetManhattanDistance(IntVec2(currentAgent.tileX, currentAgent.tileY), IntVec2(m_currentTurnInfo.observedAgents[observeIndex].tileX, m_currentTurnInfo.observedAgents[observeIndex].tileY));
		if (distance < closestDistance)
		{
			closestIndex = observeIndex;
			closestDistance = distance;
		}
	}

	return closestIndex;
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 AIPlayerController::FindClosestTile(AgentReport& currentAgent, eTileType tileType)
{
	short destX = 9999;
	short destY = 9999;
	int closestIndex = 0;

	//Look for visible tiles
	for (int dirtIndex = 0; dirtIndex < MAX_ARENA_TILES; dirtIndex++)
	{
		if (m_currentTurnInfo.observedTiles[dirtIndex] == tileType)
		{
			short dirtFoundX = 0;
			short dirtFoundY = 0;

			GetTileXYFromIndex(dirtIndex, dirtFoundX, dirtFoundY);

			short closestX = 0;
			short closestY = 0;

			ReturnClosestAmong(currentAgent, closestX, closestY, destX, destY, dirtFoundX, dirtFoundY);
			if (closestX == destX && closestY == destY)
			{
				closestIndex = dirtIndex;
			}

			destX = closestX;
			destY = closestY;
		}
	}

	return IntVec2(destX, destY);
}

//------------------------------------------------------------------------------------------------------------------------------
eOrderCode AIPlayerController::GetMoveOrderToTile(AgentReport& currentAgent, short destPosX, short destPosY)
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
eTileNeighborhood AIPlayerController::IsPositionInNeighborhood(AgentReport& currentAgent, IntVec2 position)
{
	if (currentAgent.tileX == position.x && currentAgent.tileY == position.y)
	{
		return DIRECTION_THIS_TILE;
	}
	else if (currentAgent.tileX == position.x && currentAgent.tileY == position.y + 1)
	{
		return DIRECTION_SOUTH;
	}
	else if (currentAgent.tileX == position.x && currentAgent.tileY == position.y - 1)
	{
		return DIRECTION_NORTH;
	}
	else if (currentAgent.tileX == position.x + 1 && currentAgent.tileY == position.y)
	{
		return DIRECTION_WEST;
	}
	else if (currentAgent.tileX == position.x - 1 && currentAgent.tileY == position.y)
	{
		return DIRECTION_EAST;
	}
	else
	{
		return DIRECTION_NOT_NEIGHBOR;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::SetMapCostBasedOnAntVision(eAgentType agentType, std::vector<int>& costMap)
{
	//Check what is visible in the map and set costs accordingly
	int mapSize = m_matchInfo.mapWidth * m_matchInfo.mapWidth;
	if (costMap.size() != mapSize)
	{
		costMap.resize(mapSize);
	}

	for (int observedTileIndex = 0; observedTileIndex < mapSize; observedTileIndex++)
	{
		int cost = GetTileCostForAgentType(agentType, m_currentTurnInfo.observedTiles[observedTileIndex]);
		costMap[observedTileIndex] = cost;
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
			return 10;
			break;
		case TILE_TYPE_UNSEEN:
			return 1;
			break;
		case 	TILE_TYPE_STONE:
			return 999999;
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
			return 999999;
			break;
		case TILE_TYPE_UNSEEN:
			return 1;
			break;
		case 	TILE_TYPE_STONE:
			return 999999;
			break;
		}
	}
		break;
	case AGENT_TYPE_SOLDIER:
	{
		switch (tileType)
		{
		case TILE_TYPE_DIRT:
			return 999999;
			break;
		case TILE_TYPE_AIR:
		case TILE_TYPE_CORPSE_BRIDGE:
			return 1;
			break;
		case TILE_TYPE_WATER:
			return 999999;
			break;
		case TILE_TYPE_UNSEEN:
			return 1;
			break;
		case 	TILE_TYPE_STONE:
			return 999999;
			break;
		}
	}
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsTileSafeForAgentType(eTileType tileType, eAgentType agentType)
{
	switch (agentType)
	{
	case AGENT_TYPE_QUEEN:
	{
		return IsTileSafeForQueen(tileType);
	}
		break;
	case AGENT_TYPE_SCOUT:
	{
		return IsTileSafeForScout(tileType);
	}
		break;
	case AGENT_TYPE_SOLDIER:
	{
		return IsTileSafeForSoldier(tileType);
	}
		break;
	case AGENT_TYPE_WORKER:
	{
		return IsTileSafeForWorker(tileType);
	}
		break;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsTileSafeForQueen(eTileType tileType)
{
	switch (tileType)
	{
	case TILE_TYPE_AIR:
	case TILE_TYPE_CORPSE_BRIDGE:
		return true;
	case TILE_TYPE_DIRT:
		return false;
	case TILE_TYPE_STONE:
		return false;
	case TILE_TYPE_UNSEEN:
		return false;
	case TILE_TYPE_WATER:
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsTileSafeForScout(eTileType tileType)
{
	switch (tileType)
	{
	case TILE_TYPE_AIR:
	case TILE_TYPE_CORPSE_BRIDGE:
	case TILE_TYPE_DIRT:
	case TILE_TYPE_UNSEEN:
		return true;
	case TILE_TYPE_STONE:
		return false;
	case TILE_TYPE_WATER:
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsTileSafeForSoldier(eTileType tileType)
{
	switch (tileType)
	{
	case TILE_TYPE_AIR:
	case TILE_TYPE_CORPSE_BRIDGE:
	case TILE_TYPE_UNSEEN:
		return true;
	case TILE_TYPE_DIRT:
	case TILE_TYPE_STONE:
		return false;
	case TILE_TYPE_WATER:
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsTileSafeForWorker(eTileType tileType)
{
	switch (tileType)
	{
	case TILE_TYPE_AIR:
	case TILE_TYPE_CORPSE_BRIDGE:
	case TILE_TYPE_UNSEEN:
	case TILE_TYPE_DIRT:
		return true;
	case TILE_TYPE_STONE:
		return false;
	case TILE_TYPE_WATER:
		return false;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool AIPlayerController::IsThisAgentQueen(AgentReport& report)
{
	for (int i = 0; i < m_queenReports.size(); i++)
	{
		if(m_queenReports[i].agentID == 0)
			continue;

		if (report.tileX == m_queenReports[i].tileX && report.tileY == m_queenReports[i].tileY)
			return true;
	}

	return false;
}

//------------------------------------------------------------------------------------------------------------------------------
int AIPlayerController::GetClosestQueenTileIndex(AgentReport& agentReport)
{
	int closestDistance = 99999;
	int closestIndex = 0;
	for (int i = 0; i < m_queenReports.size(); i++)
	{
		if(m_queenReports[i].agentID == 0)
			continue;

		int distance = GetManhattanDistance(IntVec2(agentReport.tileX, agentReport.tileY), IntVec2(m_queenReports[i].tileX, m_queenReports[i].tileY));
		if (distance < closestDistance)
		{
			closestIndex = i;
			distance = closestDistance;
		}
	}

	int tileIndex = GetTileIndex(m_queenReports[closestIndex].tileX, m_queenReports[closestIndex].tileY);
	return tileIndex;
}

//------------------------------------------------------------------------------------------------------------------------------
int AIPlayerController::IsEnemyInNeighborhood(int closestEnemy, AgentReport& report)
{
	int currentTileIndex = GetTileIndex(report.tileX, report.tileY);

	int inNeighborhood = -1;
	if (closestEnemy == currentTileIndex + 1)
	{
		//He's EAST
		inNeighborhood = 0;
	}
	else if (closestEnemy == currentTileIndex - 1)
	{
		//He's WEST
		inNeighborhood = 1;
	}
	else if (closestEnemy == currentTileIndex + m_matchInfo.mapWidth)
	{
		//He's NORTH
		inNeighborhood = 2;
	}
	else if(closestEnemy == currentTileIndex - m_matchInfo.mapWidth)
	{
		//He's SOUTH
		inNeighborhood = 3;
	}

	return inNeighborhood;
}