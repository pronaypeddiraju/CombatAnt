//------------------------------------------------------------------------------------------------------------------------------
#include "AIPlayerController.hpp"
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
				MoveRandom(report.agentID);
				break;

				// moves randomly, but if they fall on food, will pick it up if hands are free
			case AGENT_TYPE_WORKER:
				if (report.state == STATE_HOLDING_FOOD)
				{
					MoveRandom(report.agentID);
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
						MoveRandom(report.agentID);
					}
				}
				break;

				// Soldier randomly walks
			case AGENT_TYPE_SOLDIER:
				MoveRandom(report.agentID);
				break;

				// queen either moves or spawns randomly
			case AGENT_TYPE_QUEEN:
			{
				const float RANODM_SPAWN_CHANCE = 0.9f;
				if (RandomFloat01() < RANODM_SPAWN_CHANCE)
				{
					// spawn
					int typeOffset = rand() % 3;
					eOrderCode order = (eOrderCode)(ORDER_BIRTH_SCOUT + typeOffset);
					AddOrder(report.agentID, order);
				}
				else
				{
					MoveRandom(report.agentID);
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
void AIPlayerController::MoveRandom(AgentID agent)
{
	int offset = rand() % 4;
	eOrderCode order = (eOrderCode)(ORDER_MOVE_EAST + offset);

	AddOrder(agent, order);
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

