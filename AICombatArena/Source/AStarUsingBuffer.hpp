#pragma once
#include <vector>

extern int ExploredNodes;
extern std::vector<int> Landmarks;
extern std::vector<std::vector<int>> LD;

int AStarFindPath(const int nStartX, const int nStartY,
	const int nTargetX, const int nTargetY,
	const int* pMap, const int nMapWidth, const int nMapHeight,
	int* pOutBuffer, const int nOutBufferSize);

