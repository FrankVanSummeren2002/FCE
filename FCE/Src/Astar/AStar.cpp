#include "Core/FCEpch.h"
#include "Astar/AStar.h"
#include "queue"
#include "Core/FCEpch.h"
#include "Core/Engine.h"
#include "Header/VulkanFrontEnd.h"
#include "Core/Timing.h"
int mWidth = 0;
glm::vec3 mCellSize;
glm::vec3 mCellOffset;
std::vector<FCE::Vertex> mGrid;

FCE::AStarPoint::AStarPoint(glm::vec3 Position, int value) : mPosition{ Position }, mValue{ value }
{}

FCE::AStarPoint::AStarPoint()
{
}

void FCE::AStarSystem::FillGrid(std::vector<AStarPoint> grid, int xSize,glm::vec3 cellsize)
{
	if (xSize <= 0)
	{
		std::cout << "the width of the AStar Grid is 0" << std::endl;
		return;
	}
	mWidth = xSize;

	if (cellsize.y > 0.0001 && cellsize.z > 0.00001)
	{
		std::cout << "both the z and y value of the gridcell size are filled which is not possible" << std::endl;
		return;
	}
	mCellSize = cellsize;

	if (!grid.empty())
		mCellOffset = grid[0].mPosition - 0.5f * mCellSize;

	mGrid.clear();
	for (int i = 0; i < grid.size(); i++)
	{
		int y = floor(i / (float)mWidth);
		int x = i - y * mWidth;
		mGrid.push_back({ grid[i],x,y });
	}

	for (int i = 0; i < mGrid.size(); i++)
		UpdateNeighbours(mGrid[i]);
}

void FCE::AStarSystem::ChangeGridValue(int GridPos, int value)
{
	
}

FCE::AStarPoint FCE::AStarSystem::GetGridPoint(int gridPos)
{
	if (mGrid.size() < gridPos)
		return mGrid[gridPos].mPoint;
	return FCE::AStarPoint(glm::vec3(-2),-2);
}

int FCE::AStarSystem::ConvertWorldToGridPosition(glm::vec3 Pos)
{
	glm::vec3 pos(0);
	pos.x = (Pos.x - mCellOffset.x) / mCellSize.x;
	if (mCellSize.y > 0.0)
		pos.y = (Pos.y - mCellOffset.y) / mCellSize.y;
	if (mCellSize.z > 0.0)
		pos.z = (Pos.z - mCellOffset.z) / mCellSize.z;

	return (int)pos.x + (int)(pos.y + pos.z) * mWidth;
}

FCE::PathPoint* FCE::AStarSystem::FindPath(int gridPosStart, int gridPosEnd)
{
	//get the start and end positions
	if (gridPosEnd > mGrid.size() || gridPosStart > mGrid.size() || gridPosEnd < 0 || gridPosStart < 0)
		return nullptr;

	FCE::Vertex* end = &mGrid[gridPosEnd]; 
	std::priority_queue<FCE::Vertex*, std::vector<Vertex*>, CompareVertex> OpenList;
	OpenList.push(&mGrid[gridPosStart]);
	mGrid[gridPosStart].mPoint.heuristic = heuristics(mGrid[gridPosStart].mPoint.mPosition, end->mPoint.mPosition);
	mGrid[gridPosStart].mPoint.pathLength = mGrid[gridPosStart].mPoint.mValue;

	std::vector<FCE::AStarPoint*> ClearList;
	ClearList.push_back(&mGrid[gridPosStart].mPoint);
	
	//while their are still options
	while (!OpenList.empty())
	{
		FCE::Vertex& p = *OpenList.top();
		//TODO make sure that it does not infinite loop
		//The path has reached the end goal
		if (p.mPoint.mPosition == end->mPoint.mPosition)
		{
			FCE::PathPoint* Path = new PathPoint();
			Path->mNext = nullptr;
			Path->mPoint = &p.mPoint;
			FCE::PathPoint* Prev = nullptr;
			while (Path->mPoint->mParent != nullptr)
			{
				Prev = new PathPoint();
				Prev->mPoint = Path->mPoint->mParent;
				Prev->mNext = Path;
				Path = Prev;
			}
			//create all of the route
			for (auto it : ClearList)
				ClearSearchData(*it);
			return Path;
		}
		OpenList.pop();
		for (int i = 0; i < p.mNeighbourSize; i++)
		{
			Vertex* v = p.mNeigbours[i];
			if (v->mPoint.pathLength == 0 || p.mPoint.pathLength + v->mPoint.mValue < v->mPoint.pathLength)
			{
				v->mPoint.heuristic = heuristics(v->mPoint.mPosition, end->mPoint.mPosition);
				v->mPoint.pathLength = p.mPoint.pathLength + v->mPoint.mValue;
				v->mPoint.mParent = &p.mPoint;
				ClearList.push_back(&v->mPoint);
				OpenList.push(v);
			}
		}
	}
	return nullptr;
}

void FCE::AStarSystem::VisualiseGrid(glm::vec3 OpenColor, glm::vec3 BlockedColor)
{
	glm::vec3 halfSize = mCellSize * 0.5f;

	for (const auto& it : mGrid)
	{
		int baseX =  it.mGridPositionX;
		int BaseY =  it.mGridPositionY;

		bool Open = IsOpen(baseX, BaseY);
		//&& IsOpen(baseX + 1, BaseY) &&
		//	IsOpen(baseX-1, BaseY) && IsOpen(baseX, BaseY+1) && IsOpen(baseX, BaseY + 1);

		glm::vec3 Color;
		if (Open)
			Color = OpenColor;
		else
			Color = BlockedColor;

			if (FCE::Engine::GetRenderMode())
			{

				//FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition - halfSize, it.mPoint.mPosition + halfSize,Color,Color);
				//FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition - halfSize, it.mPoint.mPosition + halfSize, Color, Color);
				//FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition - halfSize, it.mPoint.mPosition + halfSize, Color, Color);
			}
			else
			{
				Color = Open && IsOpen(baseX, BaseY - 1) ? OpenColor : BlockedColor;
				FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition + glm::vec3(-halfSize.x, -halfSize.y, 0), it.mPoint.mPosition + glm::vec3(halfSize.x, -halfSize.y, 0), Color, Color, true);
				Color = Open && IsOpen(baseX - 1, BaseY) ? OpenColor : BlockedColor;
				FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition + glm::vec3(-halfSize.x, -halfSize.y, 0), it.mPoint.mPosition + glm::vec3(-halfSize.x, halfSize.y, 0), Color, Color, true);
				Color = Open && IsOpen(baseX, BaseY + 1) ? OpenColor : BlockedColor;
				FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition + glm::vec3(halfSize.x, halfSize.y, 0), it.mPoint.mPosition + glm::vec3(-halfSize.x, halfSize.y, 0), Color, Color, true);
				Color = Open && IsOpen(baseX + 1, BaseY) ? OpenColor : BlockedColor;
				FCE::Engine::GetRenderer()->DrawLine(it.mPoint.mPosition + glm::vec3(halfSize.x, halfSize.y, 0), it.mPoint.mPosition + glm::vec3(halfSize.x, -halfSize.y, 0), Color, Color, true);
			}
		
	}
}

void FCE::AStarSystem::VisualisePaths(glm::vec3 Color)
{
	auto view = FCE::Engine::GetRegistery()->view<AStarComponent>();
	for (auto entityHandle : view)
	{
		VisualisePath(entityHandle, Color);
	}
}

void FCE::AStarSystem::VisualisePath(entt::entity Entity, glm::vec3 Color)
{
	PathPoint* p = FCE::Engine::GetRegistery()->get<AStarComponent>(Entity).mPath;
	while (p && p->mNext)
	{
		FCE::Engine::GetRenderer()->DrawLine(p->mPoint->mPosition, p->mNext->mPoint->mPosition, Color, Color, !FCE::Engine::GetRenderMode());
		p = p->mNext;
	}
}

enum Finds
{
	RIGHT = 1 << 0,
	UP = 1 << 1,
	LEFT = 1 << 2,
	DOWN = 1 << 3
};

void FCE::AStarSystem::UpdateNeighbours(Vertex& vertex)
{
	int VecPos;
	int maxValue = mGrid.size();

	for (int i = 0; i < 8; i++)
		vertex.mNeigbours[i] = nullptr;
	vertex.mNeighbourSize = 0;

	int baseX = vertex.mGridPositionX;
	int baseY = vertex.mGridPositionY;
	
	int Flags = 0;
	//do the up down left right
	if (CheckNeighbour(baseX + 1, baseY))
	{
		AddNeighbour(vertex, baseX + 1, baseY);
		Flags |= RIGHT;
	}
	if (CheckNeighbour(baseX, baseY + 1))
	{
		AddNeighbour(vertex, baseX, baseY + 1);
		Flags |= UP;
	}
	if (CheckNeighbour(baseX - 1, baseY))
	{
		AddNeighbour(vertex, baseX - 1, baseY);
		Flags |= LEFT;
	}
	if (CheckNeighbour(baseX, baseY - 1))
	{
		AddNeighbour(vertex, baseX, baseY - 1);
		Flags |= DOWN;
	}
	//do the corners only if the rest is also both sides are clear
	if (Flags & RIGHT && Flags & UP)
		if(CheckNeighbour(baseX + 1,baseY + 1))
			AddNeighbour(vertex, baseX + 1, baseY + 1);

	if (Flags & RIGHT && Flags & DOWN)
		if (CheckNeighbour(baseX + 1, baseY - 1))
			AddNeighbour(vertex, baseX + 1, baseY - 1);

	if (Flags & LEFT && Flags & UP)
		if (CheckNeighbour(baseX - 1, baseY + 1))
			AddNeighbour(vertex, baseX - 1, baseY + 1);

	if (Flags & LEFT && Flags & DOWN)
		if (CheckNeighbour(baseX - 1, baseY - 1))
			AddNeighbour(vertex, baseX - 1, baseY - 1);
}

bool FCE::AStarSystem::CheckNeighbour(int x, int y)
{
	int VecPos = x + (y * mWidth);
	if (VecPos > 0 && VecPos < mGrid.size())
		return mGrid[VecPos].mPoint.mValue >= 0;

	return false;
}

void FCE::AStarSystem::AddNeighbour(Vertex& Owner, int x, int y)
{
	int Pos = x + y * mWidth;
	if (Owner.mNeighbourSize < 8)
	{
		Owner.mNeigbours[Owner.mNeighbourSize] = &mGrid[Pos];
		Owner.mNeighbourSize++;
	}
}

int FCE::AStarSystem::heuristics(glm::vec3 Start, glm::vec3 End)
{
	glm::vec3 L(0);
	L.x = (End.x - Start.x) / mCellSize.x;
	if (mCellSize.y > 0.0001f)
	L.y = (End.y - Start.y) / mCellSize.y;
	if(mCellSize.z > 0.0001f)
	L.z = (End.z - Start.z) / mCellSize.z;

	return round(glm::length((End - Start)));
}

void FCE::AStarSystem::ClearSearchData(AStarPoint& point)
{
	point.heuristic = 0;
	point.mParent = nullptr;
	point.pathLength = 0;
}

int FCE::AStarSystem::IsOpen(int x, int y)
{
	int arrVal = x + y * mWidth;
	if (arrVal >= mGrid.size() || x >= mWidth || x < 0 || y < 0)
		return -1;
	return mGrid[arrVal].mPoint.mValue >= 0;
}

FCE::AStarComponent::AStarComponent(float Interval,float DistanceToPoint, float PlayerDistance) : mDistanceToPoint{ DistanceToPoint }, mPlayerCheckDistance{ PlayerDistance }
{
	mInterval = FCE::TimeHandler::CreateTimer(Interval);
}

FCE::AStarComponent::AStarComponent()
{
}
