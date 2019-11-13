#pragma once
#include <corecrt_math.h>
#include <vector>
#include "IntVec2.hpp"

enum ePathState
{
	PATH_STATE_UNVISITED = 0,
	PATH_STATE_VISITED,
	PATH_STATE_FINISHED,
};

struct PathInfo_T
{
	float fCost = INFINITY;	//Actual tile cost

	float gCost = 0;	//Cost to move into
	float hCost = 0;	//Heuristic cost

	int parentIndex = -1;
	ePathState pathState = PATH_STATE_UNVISITED;
};

typedef std::vector<IntVec2> Path;
typedef std::vector<PathInfo_T> PathInfo;

class Pather
{
public:
	Path			CreatePathAStar(int startTileIndex, int endTileIndex, IntVec2 mapDimensions, const std::vector<float>& tileCosts);
	void			CalculateCostsForTileIndex(const int currentTileIndex, const int terminationPointIndex, const IntVec2& tileDimensions, const std::vector<float>& tileCosts_, bool isStart = false);
	int				SelectFromAndUpdateOpenIndexList();
	int				PopulateBoundedNeighbors(const int currentTileIdex, const IntVec2& tileDimensions, int* outNeighbors);

	IntVec2			GetTileCoordinatesFromIndex(const short tileIndex, const IntVec2 mapDims);
private:
	std::vector<int> m_openTileIndexList;
	PathInfo m_pathInfo;
};