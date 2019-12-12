#include "Agent.hpp"
#include "AIPlayerController.hpp"

//------------------------------------------------------------------------------------------------------------------------------
Agent::Agent(const AgentReport& base)
{
	UpdateAgentData(base);
	
	agentID = base.agentID;
	m_currentPath.clear();
}

//------------------------------------------------------------------------------------------------------------------------------
void Agent::UpdateAgentData(const AgentReport& agentReport)
{
	tileX = agentReport.tileX;
	tileY = agentReport.tileY;
	agentID = agentReport.agentID;

	exhaustion = agentReport.exhaustion;

	receivedCombatDamage = agentReport.receivedCombatDamage;
	receivedSuffocationDamage = agentReport.receivedSuffocationDamage;

	type = agentReport.type;
	state = agentReport.state;
	result = agentReport.result;
}

//------------------------------------------------------------------------------------------------------------------------------
bool Agent::ContinuePathIfValid()
{
	if (m_currentPath.size() > 0)
	{
		IntVec2 destination = m_currentPath.front();
		AIPlayerController* playerController = AIPlayerController::GetInstance();

		int destIndex = playerController->GetTileIndex(destination.x, destination.y);

		if (playerController->m_foodVisionHeatMap[destIndex] && type == AGENT_TYPE_WORKER)
		{
			eOrderCode order = playerController->GetMoveOrderToTile(*this, m_currentPath.back().x, m_currentPath.back().y);
			playerController->AddOrder(agentID, order);

			m_currentPath.pop_back();

			return true;
		}
		else if (type != AGENT_TYPE_WORKER)
		{
			eOrderCode order = playerController->GetMoveOrderToTile(*this, m_currentPath.back().x, m_currentPath.back().y);
			playerController->AddOrder(agentID, order);

			m_currentPath.pop_back();

			return true;
		}
		else
		{
			m_currentPath.clear();
			return false;
		}
	}
	else
	{
		return false;
	}
}
