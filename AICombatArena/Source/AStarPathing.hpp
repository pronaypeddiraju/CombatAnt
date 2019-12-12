#pragma once
#include <vector>
#include "IntVec2.hpp"

enum ePathState
{
	PATH_STATE_UNVISITED = 0,
	PATH_STATE_VISITED,
	PATH_STATE_FINISHED,
};

struct AStarPathInfo_T
{
	int fCost = INT_MAX;	//Actual tile cost

	int gCost = 0;	//Cost from tileCosts argument
	int hCost = 0;	//Heuristic cost

	int parentIndex = -1;
	ePathState pathState = PATH_STATE_UNVISITED;
};

typedef std::vector<IntVec2> Path;
typedef std::vector<AStarPathInfo_T> PathInfo;

class AStarPather
{
public:
	Path			CreatePathAStar(int startTileIndex, int endTileIndex, IntVec2 mapDimensions, const std::vector<int>& tileCosts, int limit = 256);
	bool			CalculateCostsForTileIndex(const int currentTileIndex, const int terminationPointIndex, const IntVec2& tileDimensions, const std::vector<int>& tileCosts_, bool isStart = false);
	int				SelectFromAndUpdateOpenIndexList();
	int				PopulateBoundedNeighbors(const int currentTileIdex, const IntVec2& tileDimensions, int* outNeighbors);

	IntVec2			GetTileCoordinatesFromIndex(const short tileIndex, const IntVec2 mapDims);
	
	int		m_largestOpenList = 0;
private:
	std::vector<int> m_openTileIndexList;
	PathInfo m_pathInfo;

};