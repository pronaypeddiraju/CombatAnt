//------------------------------------------------------------------------------------------------------------------------------
#include "AIPlayerController.hpp"
#include "AICommons.hpp"
#include <math.h>

AIPlayerController* g_thePlayer = nullptr;

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
	//Do whatever you need to do here to shut down safely
}

//------------------------------------------------------------------------------------------------------------------------------
void AIPlayerController::ThreadEntry(int /*threadIdx*/)
{
	// wait for data
	// process turn
	// mark turn as finished;

	ArenaTurnStateForPlayer turnState;

	while (m_running)
	{
		std::unique_lock lk(m_turnLock);
		m_turnCV.wait(lk, [&]() { return !m_running || m_lastTurnProcessed != m_currentTurnInfo.turnNumber; });

		if (m_running)
		{
			turnState = m_currentTurnInfo;
			lk.unlock();

			// process a turn and then mark that the turn is ready; 
			ProcessTurn(turnState);

			// notify the turn is ready; 
			m_lastTurnProcessed = turnState.turnNumber;
			m_debugInterface->LogText("AIPlayer Turn Complete: %i", turnState.turnNumber);
		}
	}
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
							MoveRandom(report);
						}
						else if(report.tileX == m_queenReport.tileX || report.tileY == m_queenReport.tileY)
						{
							MoveToQueen(report);
						}
						else
						{
							MoveRandom(report);
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
							MoveToClosestFood(report);
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
				const float RANDOM_SPAWN_CHANCE = 0.5f;
				if (RandomFloat01() < RANDOM_SPAWN_CHANCE)
				{
					// spawn
					if (m_numWorkers < MAX_WORKERS && report.exhaustion == 0 && m_currentTurnInfo.currentNutrients > MIN_NUTRIENTS_TO_SPAWN)
					{
						int typeOffset = rand() % 3;
						eOrderCode order = (eOrderCode)(ORDER_BIRTH_WORKER);
						AddOrder(report.agentID, order);
						m_numWorkers++;
					}
				}
			} break;
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
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER)
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
		if (m_currentTurnInfo.observedTiles[index] == TILE_TYPE_UNSEEN || m_currentTurnInfo.observedTiles[index] == TILE_TYPE_WATER)
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

