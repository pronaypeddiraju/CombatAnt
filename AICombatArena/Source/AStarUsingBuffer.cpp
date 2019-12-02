#include "AStarUsingBuffer.hpp"

#include <queue>
#include <tuple>

int AStarFindPath(const int nStartX, const int nStartY,
	const int nTargetX, const int nTargetY,
	const int* pMap, const int nMapWidth, const int nMapHeight,
	int* pOutBuffer, const int nOutBufferSize) 
{

	auto idx = [nMapWidth](int x, int y) 
	{
		return x + y * nMapWidth;
	};

	auto h = [=](int u) -> int 
	{	
		// lower bound distance to target from u
		int x = u % nMapWidth, y = u / nMapWidth;
		return abs(x - nTargetX) + abs(y - nTargetY);
	};

	const int n = nMapWidth * nMapHeight;
	const int startPos = idx(nStartX, nStartY), targetPos = idx(nTargetX, nTargetY);

	int discovered = 0; 
	int ExploredNodes = 0;
	std::vector<int> p(n), d(n, INT_MAX);
	std::priority_queue<std::tuple<int, int, int>, std::vector<std::tuple<int, int, int>>, std::greater<std::tuple<int, int, int>>> pq; // A* with tie breaking
	
	d[startPos] = 0;
	pq.push(std::make_tuple(0 + h(startPos), 0, startPos));
	
	while (!pq.empty()) 
	{
		int u = std::get<2>(pq.top()); pq.pop(); ExploredNodes++;
		for (auto e : { +1, -1, +nMapWidth, -nMapWidth }) 
		{
			int v = u + e;
			if ((e == 1 && (v % nMapWidth == 0)) || (e == -1 && (u % nMapWidth == 0)))
				continue;
			if (0 <= v && v < n && d[v] > d[u] + 1 && pMap[v]) 
			{
				p[v] = u;
				d[v] = d[u] + 1;
				if (v == targetPos)
					goto end;
				pq.push(std::make_tuple(d[v] + h(v), ++discovered, v));
			}
		}
	}
end:

	if (d[targetPos] == INT_MAX) 
	{
		return -1;
	}
	else if (d[targetPos] <= nOutBufferSize) 
	{
		int curr = targetPos;
		for (int i = d[targetPos] - 1; i >= 0; i--) 
		{
			pOutBuffer[i] = curr;
			curr = p[curr];
		}
		return d[targetPos];
	}

	return d[targetPos]; // buffer size too small
}