#pragma once
#include "Core/Timing.h"
namespace FCE
{
	//has a direction, speed, damping and clamp value
	struct VelocityComponent
	{
		glm::vec3 mDir{};
		float mSpeed{};
		float mDamping{};
		bool mPhysicsBased{false};
		VelocityComponent() {};
		VelocityComponent(float damping, float speed = 0, glm::vec3 dir = { 0,0,0 },bool PhysicsBased = false)
			: mDamping{ damping }, mSpeed{ speed }, mDir{ dir }, mPhysicsBased{PhysicsBased}{};
	};

	
	enum SteeringForces
	{
		SF_SEPARATION = 0,
		SF_ALLIGNMENT = 1,
		SF_COHESION = 2,
		SF_WALLAVOIDANCE = 3,
		SF_SMOOTHING = 4,
		SF_LAST = SF_SMOOTHING
	};
	enum SteeringForceFlags
	{
		DISABLE_SEPARATION = 0x1 << 0,
		DISABLE_COHESION = 0x1 << 1,
		DISABLE_ALLIGNMENT  = 0x1 << 2,
		DISABLE_WALLAVOIDANCE = 0x1 << 3,
		DISABLE_SMOOTHING = 0x1 << 4
	};
	struct SteeringForce
	{
		//only for wall avoidance
		Timer* mDelay;
		float mDistToCheck{ 0 };
		//

		uint32_t mFlags;
		float mMaxSpeed{0};

		glm::vec3 mDir[SF_LAST + 1] = { glm::vec3(0,0,0) };
		float mSpeed[SF_LAST + 1] = {0.f};
		float mPriority[SF_LAST + 1];
		SteeringForce(uint32_t flags, float MaxSpeed, float Priority[SF_LAST + 1], float Delay, float DistanceToCheck)
		: mMaxSpeed{MaxSpeed}, mFlags { flags }, mDistToCheck{ DistanceToCheck }
		{
			mDelay = TimeHandler::CreateTimer(Delay);
			for (int i = 0; i <= SF_LAST; i++)
				mPriority[i] = Priority[i];
		};
	};
	//can lock the movement if it is to far away from the target is to far away
	struct MovementLock
	{
		float mDist;
		bool mLocked{ false };
		entt::entity mTarget;
		MovementLock(float distanceToLock, entt::entity target) : mDist{ distanceToLock }, mTarget{target} {};
	};
}