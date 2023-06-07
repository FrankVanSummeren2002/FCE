#pragma once
namespace FCE
{
	struct AStarComponent;
	struct Timer;

	struct AStarPoint
	{
		int mValue;
		glm::vec3 mPosition;
		//TODO these three should not be in here
		int pathLength = 0;
		int heuristic = 0;
		AStarPoint* mParent = nullptr;

		AStarPoint(glm::vec3 Position, int value);
		AStarPoint();
	};
	struct PathPoint
	{
		AStarPoint* mPoint;
		PathPoint* mNext;
	};
	struct Vertex
	{
		AStarPoint mPoint;
		int mNeighbourSize = 0;
		int mGridPositionX;
		int mGridPositionY;
		Vertex* mNeigbours[8] = { nullptr };
		Vertex(AStarPoint point, int GridPositionX, int GridPositionY) { mPoint = point; mGridPositionX = GridPositionX; mGridPositionY = GridPositionY; };
	};	
	struct CompareVertex
	{
		bool operator()(Vertex const& p1, Vertex const& p2)
		{
			return p1.mPoint.pathLength + p1.mPoint.heuristic > p2.mPoint.pathLength + p2.mPoint.heuristic;
		}
		bool operator()(Vertex* const& p1, Vertex* const& p2)
		{
			return p1->mPoint.pathLength + p1->mPoint.heuristic > p2->mPoint.pathLength + p2->mPoint.heuristic;
		}
	};
	class AStarSystem
	{
	private:

	public:

	private:
		//update all of the posible neigbours of a vertex
		static void UpdateNeighbours(Vertex& vertex);
		//See if their is a posible neigbour
		static bool CheckNeighbour(int x, int y);
		//adds a neighbour to the vertex
		static void AddNeighbour(Vertex& Owner, int x, int y);
		static int heuristics(glm::vec3 Start, glm::vec3 End);
		//Clear the search data of the points after the FindPath is done
		static void ClearSearchData(AStarPoint& point);

		//Checks wheter a grid value is lower then 0 or not
		//will return 0 closed
		//will return 1 when open
		//will return -1 if it is outside of the map
		static int IsOpen(int x, int y);
	public:
		//The grid is a 1d vector where you have to specify the height of the grid by a yValue
		//The value of an edge will be the mix of the two values of the cells
		//the cellsize is the size of an individual grid. The x should always be a value > 0 and
		//since the grid is 2d, either the y or the z value should be filled. If both are filled the function will fail
		//The first value should be the top left corner of the grid
		static void FillGrid(std::vector<AStarPoint> grid, int xSize,glm::vec3 cellSize);
		//change grid value should be used sparingly as it has to update all of the existing paths
		//TODO DOESNT WORK YET
		static void ChangeGridValue(int GridPos, int value);
		//returns -2 if it couldnt find anything
		static AStarPoint GetGridPoint(int gridPos);
		//converts a world position into a position of the grid
		//will return -1 if it is not on the grid
		//Make sure that the value is placed inside of the grid
		static int ConvertWorldToGridPosition(glm::vec3 Pos);
		static PathPoint* FindPath(int gridPosStart,int gridPosEnd);
		//visualise all of the paths with the same color
		static void VisualisePaths(glm::vec3 Color);
		static void VisualisePath(entt::entity Entity, glm::vec3 Color);
		//visualises the grid
		//Open color is the color of all of the boxes that have a value >= 0
		static void VisualiseGrid(glm::vec3 OpenColor, glm::vec3 BlockedColor);
	};

	struct AStarComponent
	{
		FCE::PathPoint* mPath = nullptr;
		FCE::Timer* mInterval = nullptr;
		float mDistanceToPoint;
		float mPlayerCheckDistance;
		//The DistanceToPoint is how close the AI has to move to the point to start going to the next
		//The PlayerDistance is the distance to check for the Player If it can find the player then it stops using PathFinding and move straight to the target
		AStarComponent(float IntervalTime, float DistanceToPoint, float PlayerDistance);
		AStarComponent();
	};
}