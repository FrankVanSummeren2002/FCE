#pragma once
namespace FCE
{
	class EnemySystem
	{
	public:	
		//give an interval between the updating of the groupings of enemies
		//give the flags for the objects the wall avoidance can collide with
		static void Init(float Interval,uint32_t wallAvoidanceFlags);
		static void Update(float DT);
		//force the update of the groupings heavy performance penalty
		//only adviced usage would be after a big batch of enemies have spawned in a frame
		static void ForceUpdateGroupings();
	private:
		static void UpdateGroups();
	};
}