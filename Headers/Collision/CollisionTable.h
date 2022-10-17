#pragma once
#include "iostream"
#include "vector"
namespace FCE
{
	struct CollisionMaskCell
	{
		uint32_t X{ 0 };
		uint32_t Y{ 0 };

		CollisionMaskCell() {};
		CollisionMaskCell(uint32_t row, uint32_t coll) : X(row), Y(coll) {};

		bool operator== (CollisionMaskCell A)
		{
			return X == A.X && Y == A.Y;
		}
		bool Contains(uint32_t identifier, uint32_t& other)
		{
			if (identifier == X) { other = Y; return true; }
			else if (identifier == Y) { other = X; return true; }
			return false;
		}
	};

	//containts all of the collisions it should collide with
	struct CollisionMaskTable
	{
		bool Add(CollisionMaskCell cell)
		{
			//make sure no dups happen
			for (auto it : mCells)
			{
				if (it == cell)
					return false;
			}
			mCells.push_back(cell);
			return true;
		}
		//add two items who should collide with eachother in case of an overlap
		bool Add(uint32_t row, uint32_t collum)
		{
			return Add(CollisionMaskCell(row, collum));
		}

		//find all of the collisions this identifier masks against 
		uint32_t Find(uint32_t identifier)
		{
			uint32_t other;
			uint32_t result{0};
			for (auto it : mCells)
			{
				if (it.Contains(identifier, other))
					result |= other;
			}
			return result;
		}

		void Clear() { mCells.clear(); }
	private:
		std::vector<CollisionMaskCell> mCells;
	};
}