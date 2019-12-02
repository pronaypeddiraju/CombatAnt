#include "AStarPathing.hpp"
#include "MathUtils.hpp"
#include "ErrorWarningAssert.hpp"

Path AStarPather::CreatePathAStar(int startTileIndex, int endTileIndex, IntVec2 mapDimensions, const std::vector<int>& tileCosts)
{
	// m_pathInfo lives on Pather. It is the current std::vector<PathInfo>

	m_pathInfo.clear();
	m_pathInfo.resize(mapDimensions.x * mapDimensions.y);

	CalculateCostsForTileIndex(startTileIndex, endTileIndex, mapDimensions, tileCosts, true);

	// Begin the AStar!
	while (m_openTileIndexList.size() > 0 && m_openTileIndexList.size() < 100)
	{
		int currentIndex = SelectFromAndUpdateOpenIndexList();
		m_pathInfo[currentIndex].pathState = PATH_STATE_FINISHED;

		// If we have our Termination Index, we know we have the shortest path set already;
		if (currentIndex == endTileIndex)
		{
			break;
		}

		int neighbors[4] = { 0, 0, 0, 0 };
		int validNeighbors = PopulateBoundedNeighbors(currentIndex, mapDimensions, neighbors);

		for (int i = 0; i < validNeighbors; i++)
		{
			bool canPath = CalculateCostsForTileIndex(neighbors[i], endTileIndex, mapDimensions, tileCosts);
			if (canPath)
			{
				m_pathInfo[neighbors[i]].parentIndex = currentIndex;

				if (m_pathInfo[neighbors[i]].pathState == PATH_STATE_UNVISITED)
				{
					m_pathInfo[neighbors[i]].pathState = PATH_STATE_VISITED;
					m_openTileIndexList.push_back(neighbors[i]);
				}
			}
			/*
			//When the path cost is too high return an empty path
			else
			{
				//The cost is too high to path
				Path path;
				return path;
			}
			*/
		}
	}

	DebuggerPrintf("\n %d", m_openTileIndexList.size());
	if (m_openTileIndexList.size() > m_largestOpenList)
	{
		m_largestOpenList = m_openTileIndexList.size();
	}

	// Work backwards from our Termination Point to the Starting Point;
	Path path;
	IntVec2 pathCoord = GetTileCoordinatesFromIndex(endTileIndex, mapDimensions);
	path.push_back(pathCoord);

	int nextIndex = m_pathInfo[endTileIndex].parentIndex;

	bool workingBackwards = true;
	while (workingBackwards)
	{
		//pathCoord = GetCoordFromIndex(nextIndex, mapDimensions);
		//path->push_back(pathCoord);

		if (nextIndex == -1 || nextIndex == startTileIndex)
		{
			//pathCoord = GetCoordFromIndex(nextIndex, mapDimensions);
			workingBackwards = false;
		}
		else
		{
			pathCoord = GetTileCoordinatesFromIndex(nextIndex, mapDimensions);
			path.push_back(pathCoord);
			nextIndex = m_pathInfo[nextIndex].parentIndex;
		}
	}

	m_openTileIndexList.clear();

	return path;
}

//------------------------------------------------------------------------------------------------------------------------------
int AStarPather::SelectFromAndUpdateOpenIndexList()
{
	// Get my Best Index from the Open List and then remove it from the Open List;
	int counter = 0;
	int eraseSlot = 0;
	int smallestCostIndex = m_openTileIndexList.front();
	int smallestCost = m_pathInfo[smallestCostIndex].fCost;

	for (int tileIndex : m_openTileIndexList)
	{
		if(tileIndex < 0)
			continue;

		if (m_pathInfo[tileIndex].fCost < smallestCost)
		{
			smallestCostIndex = tileIndex;
			smallestCost = m_pathInfo[tileIndex].fCost;
			eraseSlot = counter;
		}
		counter++;
	}

	m_openTileIndexList.erase(m_openTileIndexList.begin() + eraseSlot);

	// Set the state of this index to be finished, we are going to process it now;
	m_pathInfo[smallestCostIndex].pathState = PATH_STATE_FINISHED;

	return smallestCostIndex;
}

//------------------------------------------------------------------------------------------------------------------------------
bool IsContained(const IntVec2 tile, const IntVec2& dimensions)
{
	if (tile.x < dimensions.x
		&& tile.x >= 0
		&& tile.y < dimensions.y
		&& tile.y >= 0)
	{
		return true;
	}

	return false;
}

//------------------------------------------------------------------------------------------------------------------------------
int AStarPather::PopulateBoundedNeighbors(const int currentTileIdex, const IntVec2& tileDimensions, int* outNeighbors)
{
	IntVec2 currentTile = GetTileCoordinatesFromIndex(currentTileIdex, tileDimensions);
	IntVec2 northTile = IntVec2(currentTile.x, currentTile.y + 1);
	IntVec2 southTile = IntVec2(currentTile.x, currentTile.y - 1);
	IntVec2 eastTile = IntVec2(currentTile.x + 1, currentTile.y);
	IntVec2 westTile = IntVec2(currentTile.x - 1, currentTile.y);

	int north = currentTileIdex + tileDimensions.x;
	int south = currentTileIdex - tileDimensions.x;
	int east = currentTileIdex + 1;
	int west = currentTileIdex - 1;

	int i = 0;
	if (IsContained(northTile, tileDimensions) && m_pathInfo[north].pathState == PATH_STATE_UNVISITED)
	{
		outNeighbors[i] = north;
		i++;
	}
	if (IsContained(southTile, tileDimensions) && m_pathInfo[south].pathState == PATH_STATE_UNVISITED)
	{
		outNeighbors[i] = south;
		i++;
	}
	if (IsContained(eastTile, tileDimensions) && m_pathInfo[east].pathState == PATH_STATE_UNVISITED)
	{
		outNeighbors[i] = east;
		i++;
	}
	if (IsContained(westTile, tileDimensions) && m_pathInfo[west].pathState == PATH_STATE_UNVISITED)
	{
		outNeighbors[i] = west;
		i++;
	}

	return i;
}

//------------------------------------------------------------------------------------------------------------------------------
IntVec2 AStarPather::GetTileCoordinatesFromIndex(const short tileIndex, const IntVec2 mapDims)
{
	IntVec2 tileCoords;
	tileCoords.x = tileIndex % mapDims.x;
	tileCoords.y = tileIndex / mapDims.x;
	return tileCoords;
}

//------------------------------------------------------------------------------------------------------------------------------
bool AStarPather::CalculateCostsForTileIndex(const int currentTileIndex, const int terminationPointIndex, const IntVec2& mapDimensions, const std::vector<int>& tileCosts_, bool isStart /*= false*/)
{
	if (!isStart)
	{
		m_pathInfo[currentTileIndex].gCost = tileCosts_[currentTileIndex];
		m_pathInfo[currentTileIndex].hCost = GetManhattanDistance(GetTileCoordinatesFromIndex(currentTileIndex, mapDimensions), GetTileCoordinatesFromIndex(terminationPointIndex, mapDimensions));
		m_pathInfo[currentTileIndex].fCost = m_pathInfo[currentTileIndex].gCost + m_pathInfo[currentTileIndex].hCost;

		if (m_pathInfo[currentTileIndex].fCost < 99999)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		m_pathInfo[currentTileIndex].gCost = tileCosts_[currentTileIndex];
		m_pathInfo[currentTileIndex].hCost = GetManhattanDistance(GetTileCoordinatesFromIndex(currentTileIndex, mapDimensions), GetTileCoordinatesFromIndex(terminationPointIndex, mapDimensions));
		m_pathInfo[currentTileIndex].fCost = 0;
		m_pathInfo[currentTileIndex].pathState = PATH_STATE_FINISHED;

		int neighbors[4] = { 0, 0, 0, 0 };
		int validNeighbors = PopulateBoundedNeighbors(currentTileIndex, mapDimensions, neighbors);

		for (int i = 0; i < validNeighbors; i++)
		{
			CalculateCostsForTileIndex(neighbors[i], terminationPointIndex, mapDimensions, tileCosts_);
			m_pathInfo[neighbors[i]].parentIndex = currentTileIndex;

			if (m_pathInfo[neighbors[i]].pathState == PATH_STATE_UNVISITED)
			{
				m_pathInfo[neighbors[i]].pathState = PATH_STATE_VISITED;
				m_openTileIndexList.push_back(neighbors[i]);
			}
		}

		return true;
	}
}