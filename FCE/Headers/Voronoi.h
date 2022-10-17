#pragma once

namespace FCE
{
	struct CellTable
	{
		
	};
	struct DoubleLinked
	{
		DoubleLinked* prev;
		DoubleLinked* next;
		glm::vec2 val;

	};

	class Voronoi
	{
		glm::vec2 mMax;
		std::vector<glm::vec2> mGeneratedPoints;
		std::vector<glm::vec2> mQueue;
		void GeneratePoints(int PointCount,glm::vec2 max);
	};
}