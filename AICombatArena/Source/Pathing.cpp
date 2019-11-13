//------------------------------------------------------------------------------------------------------------------------------
#include "Pathing.hpp"
#include "RandomNumberGenerator.hpp"
#include "AICommons.hpp"
#include "ErrorWarningAssert.hpp"

extern RandomNumberGenerator* g_RNG;

//------------------------------------------------------------------------------------------------------------------------------
void Pather::Init(const IntVec2& mapSize, float initialCost)
{
	m_costs.Init(mapSize, initialCost);
}

//------------------------------------------------------------------------------------------------------------------------------
void Pather::SetAllCosts(float cost)
{
	m_costs.SetAll(cost);
}

//------------------------------------------------------------------------------------------------------------------------------
void Pather::SetCost(const IntVec2& cell, float cost)
{
	IntVec2 bounds = m_costs.GetSize();
	if (!cell.IsInBounds(bounds))
		return;

	m_costs.Set(cell, cost);
}

//------------------------------------------------------------------------------------------------------------------------------
void Pather::AddCost(const IntVec2& cell, float costToAdd)
{
	float currentCost = m_costs.Get(cell);
	float newCost = currentCost + costToAdd;
	m_costs.Set(cell, newCost);
}

//------------------------------------------------------------------------------------------------------------------------------
// Method to create the distance field based on costs
//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::StartDistanceField(Pather* pather, Path* unitPath)
{
	m_visited.clear();
	m_pather = pather;
	PathInfo_T info;
	m_pathInfo.Init(m_pather->m_costs.GetSize(), info);

	//Make array for neighbor tiles and an openList vector
	std::vector<PathInfo_T> neighbors(4);
	std::vector<PathInfo_T> openList;

	//Push the seed into openList
	info.cost = m_pather->m_costs.Get(m_endPoint);
	info.parent = m_endPoint;
	openList.push_back(info);

	//Run Dijkstra Path Finder
	while (!openList.empty())
	{
		//Get cell with minimum cost and delete from set
		PathInfo_T lowestCostCell = PopLowestCostCellFromList(openList);

		//Visit the lowest cost cell
		m_visited.push_back(lowestCostCell);
		info.cost = m_pather->m_costs.Get(lowestCostCell.parent) + lowestCostCell.cost;
		info.state = PATH_STATE_VISITED;
		info.parent = lowestCostCell.parent;
		m_pathInfo.Set(lowestCostCell.parent, info);

		if (info.parent == m_startPoint)
		{
			break;
		}

		//Get neighbors and push into open list
		GetNeighbors(lowestCostCell, neighbors);
		SetNeighborCosts(neighbors, lowestCostCell.cost);
		PushToOpenList(neighbors, openList);
	}

	FallDownToShortestPath(*unitPath);
}

//------------------------------------------------------------------------------------------------------------------------------
PathInfo_T PathSolver::PopLowestCostCellFromList(std::vector<PathInfo_T>& openList)
{
	float lowestCost = 1000000.f;
	PathInfo_T lowestCostCell;
	lowestCostCell.parent = IntVec2(-1, -1);
	int lowestIndex = 0;
	for (int index = 0; index < openList.size(); index++)
	{
		if (openList[index].parent == IntVec2(-1, -1))
			continue;

		if (openList[index].cost < lowestCost)
		{
			lowestCost = openList[index].cost;
			lowestCostCell = openList[index];
			lowestIndex = index;
		}
	}

	openList.erase(openList.begin() + lowestIndex);
	return lowestCostCell;
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::PushToOpenList(const std::vector<PathInfo_T>& neighbors, std::vector<PathInfo_T>& openList)
{
	for (int index = 0; index < 4; index++)
	{
		if (neighbors[index].parent != IntVec2(-1, -1))
		{
			if (std::find(openList.begin(), openList.end(), neighbors[index]) == openList.end())
			{
				if (m_pathInfo.Get(neighbors[index].parent).state == PATH_STATE_UNVISITED)
				{
					openList.push_back(neighbors[index]);
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::GetNeighbors(const PathInfo_T& cell, std::vector<PathInfo_T>& neighbors)
{
	PathInfo_T neighbor;
	neighbor.parent = cell.parent;

	//Left neighbor
	neighbor.parent.x -= 1;
	neighbor.cost = m_pather->m_costs.Get(cell.parent);
	SetNeighbor(neighbor, neighbors, 0);

	//Right neighbor
	neighbor = cell;
	neighbor.parent.x += 1;
	neighbor.cost = m_pather->m_costs.Get(cell.parent);
	SetNeighbor(neighbor, neighbors, 1);

	//Top neighbor
	neighbor = cell;
	neighbor.parent.y += 1;
	neighbor.cost = m_pather->m_costs.Get(cell.parent);
	SetNeighbor(neighbor, neighbors, 2);

	//Bottom neighbor
	neighbor = cell;
	neighbor.parent.y -= 1;
	neighbor.cost = m_pather->m_costs.Get(cell.parent);
	SetNeighbor(neighbor, neighbors, 3);
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::GetNeighborsFromVisited(const PathInfo_T& cell, std::vector<PathInfo_T>& neighbors)
{
	PathInfo_T neighbor;
	PathInfo_T defaultInfo;
	neighbor.parent = cell.parent;
	IntVec2 bounds = m_pather->m_costs.GetSize();

	//Left neighbor
	neighbor.parent.x -= 1;
	if (neighbor.parent.IsInBounds(bounds))
	{
		neighbor.cost = m_pathInfo.Get(neighbor.parent).cost;
		SetNeighbor(neighbor, neighbors, 0);
	}
	else
	{

		SetNeighbor(defaultInfo, neighbors, 0);
	}

	//Right neighbor
	neighbor = cell;
	neighbor.parent.x += 1;
	if (neighbor.parent.IsInBounds(bounds))
	{
		neighbor.cost = m_pathInfo.Get(neighbor.parent).cost;
		SetNeighbor(neighbor, neighbors, 1);
	}
	else
	{

		SetNeighbor(defaultInfo, neighbors, 1);
	}

	//Top neighbor
	neighbor = cell;
	neighbor.parent.y += 1;
	if (neighbor.parent.IsInBounds(bounds))
	{
		neighbor.cost = m_pathInfo.Get(neighbor.parent).cost;
		SetNeighbor(neighbor, neighbors, 2);
	}
	else
	{

		SetNeighbor(defaultInfo, neighbors, 2);
	}

	//Bottom neighbor
	neighbor = cell;
	neighbor.parent.y -= 1;
	if (neighbor.parent.IsInBounds(bounds))
	{
		neighbor.cost = m_pathInfo.Get(neighbor.parent).cost;
		SetNeighbor(neighbor, neighbors, 3);
	}
	else
	{

		SetNeighbor(defaultInfo, neighbors, 3);
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::GetCheapestNeighbors(std::vector<PathInfo_T>& cheapestCostCells, const std::vector<PathInfo_T>& list)
{
	float lowestCost = 1000000.f;

	for (int index = 0; index < list.size(); ++index)
	{
		if (list[index].parent == IntVec2(-1, -1))
			continue;

		if (m_pathInfo.Get(list[index].parent).cost <= lowestCost)
		{
			lowestCost = m_pathInfo.Get(list[index].parent).cost;
			cheapestCostCells.push_back(list[index]);
		}
	}

	//Remove anything that has a cost less than the lowestCost 
	for (int cheapestListIndex = 0; cheapestListIndex < (int)cheapestCostCells.size(); ++cheapestListIndex)
	{
		if (cheapestCostCells[cheapestListIndex].cost > lowestCost)
		{
			cheapestCostCells.erase(cheapestCostCells.begin() + cheapestListIndex);
			--cheapestListIndex;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::SetNeighbor(const PathInfo_T neighbor, std::vector<PathInfo_T>& neighborsArray, int index)
{
	if (neighbor.parent.IsInBounds(m_pather->m_costs.GetSize()))
	{
		neighborsArray[index] = neighbor;
	}
	else
	{
		PathInfo_T info;
		neighborsArray[index] = info;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::SetNeighborCosts(std::vector<PathInfo_T>& neighborsArray, float previousTileCost)
{
	for (int index = 0; index < 4; index++)
	{
		if (neighborsArray[index].parent != IntVec2(-1, -1))
		{
			neighborsArray[index].cost = m_pather->m_costs.Get(neighborsArray[index].parent) + previousTileCost;
		}

		//If the neighbor has been visited already, update his cost
		for (int visitedIndex = 0; visitedIndex < (int)m_visited.size(); visitedIndex++)
		{
			std::vector<PathInfo_T>::iterator neighborVisited = std::find(m_visited.begin(), m_visited.end(), neighborsArray[index]);
			if (neighborVisited != m_visited.end())
			{
				if (neighborVisited->cost > neighborsArray[index].cost)
				{
					neighborVisited->cost = neighborsArray[index].cost;
					neighborVisited->parent = neighborsArray[index].parent;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::AddEnd(const IntVec2& tile)
{
	m_endPoint = tile;
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::AddStart(const IntVec2& tile)
{
	m_startPoint = tile;
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::FallDownToShortestPath(Path& shortestPath)
{
	//m_endPoint;

	shortestPath.push_back(m_startPoint);
	std::vector<PathInfo_T> neighbors(4);
	std::vector<PathInfo_T> lowestCostCells;
	PathInfo_T lowestCostCell;

	PathInfo_T lastCell = m_pathInfo.Get(m_startPoint);

	while (shortestPath[(int)shortestPath.size() - 1] != m_endPoint)
	{
		lowestCostCells.clear();

		//Get neighbors
		GetNeighborsFromVisited(lastCell, neighbors);
		RemoveNeighborsIfInList(neighbors, shortestPath);
		GetCheapestNeighbors(lowestCostCells, neighbors);

		if ((int)lowestCostCells.size() > 1)
		{
			//Pick one of the cells
			int randomIndex = g_RNG->GetRandomIntInRange(0, (int)lowestCostCells.size() - 1);
			lowestCostCell = lowestCostCells[randomIndex];
		}
		else if (lowestCostCells.size() == 0)
		{
			ERROR_RECOVERABLE("There are no lowest cost cells");
		}
		else
		{
			lowestCostCell = lowestCostCells[0];
		}

		if (std::find(shortestPath.begin(), shortestPath.end(), lowestCostCell.parent) == shortestPath.end())
		{
			shortestPath.push_back(lowestCostCell.parent);
		}

		lastCell = lowestCostCell;
	}
}

//------------------------------------------------------------------------------------------------------------------------------
void PathSolver::RemoveNeighborsIfInList(std::vector<PathInfo_T>& neighbors, std::vector<IntVec2>& shortestPath)
{
	for (int index = 0; index < (int)neighbors.size(); ++index)
	{
		if (std::find(shortestPath.begin(), shortestPath.end(), neighbors[index].parent) != shortestPath.end())
		{
			//Reset this element as we don't need it
			PathInfo_T info;
			neighbors[index] = info;
		}
	}
}

//------------------------------------------------------------------------------------------------------------------------------
bool PathInfo_T::operator==(const PathInfo_T& compare) const
{
	if (cost == compare.cost && parent == compare.parent && state == compare.state)
	{
		return true;
	}
	else
	{
		return false;
	}
}
