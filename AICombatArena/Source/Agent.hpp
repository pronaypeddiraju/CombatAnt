#pragma once
#include "ArenaPlayerInterface.hpp"
#include <vector>
#include "IntVec2.hpp"

typedef std::vector<IntVec2> Path;

//------------------------------------------------------------------------------------------------------------------------------
class Agent : public AgentReport
{
public:
	Agent(const AgentReport& base);

	void UpdateAgentData(const AgentReport& agentReport);
	bool ContinuePathIfValid();

public:
	Path		m_currentPath;
};