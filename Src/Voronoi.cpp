#include "Core/FCEpch.h"
#include "Voronoi.h"

bool CompareY(glm::vec2 a, glm::vec2 b)
{
	return a.y < b.y;
}

void FCE::Voronoi::GeneratePoints(int PointCount,glm::vec2 max)
{
	mMax = max;
	for (int i = 0; i < PointCount; i++)
	{
		float X = rand() % ((int)mMax.x - 1) + rand() / RAND_MAX;
		float Y = rand() % ((int)mMax.y - 1) + rand() / RAND_MAX;

		mGeneratedPoints.push_back(glm::vec2(X, Y));
	}

	mQueue = mGeneratedPoints;
	std::sort(mQueue.begin(), mQueue.end(), CompareY);
}