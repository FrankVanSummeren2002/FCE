#include "Core/FCEpch.h"
#include "Core/Core.h"
#include "Core/Engine.h"

#include "Components/EnemyComponent.h"
#include "Components/CommonComponents.h"
#include "Components/MovementComponent.h"

#include "Systems/EnemySystem.h"
#include "Systems/CollisionSystem.h"

#include "Header/VulkanFrontEnd.h"

FCE::Timer* gInterval;
uint32_t gWallAvoindanceFlags;
std::vector<std::vector<entt::entity>> mGroups;

void Seperation()
{
	auto ECS = FCE::Engine::GetRegistery();
	auto view = ECS->view<FCE::EnemyComponent, FCE::SteeringForce,FCE::Transform>();
	for (auto [handle, EnemyComp, seperation, transform] : view.each())
	{
		if (seperation.mFlags & FCE::DISABLE_SEPARATION)
			continue;

		seperation.mDir[FCE::SF_SEPARATION] = glm::vec3(0, 0, 0);
		if (EnemyComp.groupings == -1 || mGroups[EnemyComp.groupings].size() <= 1)
			continue;

		for (auto otherHandle : mGroups[EnemyComp.groupings])
		{
			auto otherTransform = ECS->try_get<FCE::Transform>(otherHandle);

			if (otherTransform)
			{
				if ((handle != otherHandle))
				{
					glm::vec3 ToAgent = otherTransform->mTransform - transform.mTransform;
					seperation.mDir[FCE::SF_SEPARATION] += normalize(ToAgent) / length(ToAgent);
					
				}
				
			}
		}
		seperation.mSpeed[FCE::SF_SEPARATION] = glm::length(seperation.mDir[FCE::SF_SEPARATION]);
		seperation.mDir[FCE::SF_SEPARATION] = glm::normalize(seperation.mDir[FCE::SF_SEPARATION]);
	}


}
void Cohesion()
{
	auto ECS = FCE::Engine::GetRegistery();
	auto view = ECS->view<FCE::EnemyComponent, FCE::SteeringForce, FCE::Transform>();

	for (auto [handle, EnemyComp, cohesion, transform] : view.each())
	{
		if (cohesion.mFlags & FCE::DISABLE_COHESION)
			continue;

		cohesion.mDir[FCE::SF_COHESION] = glm::vec3(0, 0, 0);
		if (EnemyComp.groupings == -1 || mGroups[EnemyComp.groupings].size() <= 1)
			continue;

		glm::vec3 CenterOfMass{ 0,0,0 };

		for (auto otherHandle : mGroups[EnemyComp.groupings])
		{
			auto otherTransform = ECS->try_get<FCE::Transform>(otherHandle);
			if(otherTransform)
			CenterOfMass += otherTransform->mTransform;
		}
		CenterOfMass /= (float)(mGroups[EnemyComp.groupings].size() - 1);
		glm::vec3 DesiredVelocity = glm::normalize(CenterOfMass - transform.mTransform);
		auto vel = ECS->try_get<FCE::VelocityComponent>(handle);
		cohesion.mDir[FCE::SF_COHESION] = DesiredVelocity - vel->mDir * vel->mSpeed;

		cohesion.mSpeed[FCE::SF_COHESION] = glm::length(cohesion.mDir[FCE::SF_COHESION]);
		cohesion.mDir[FCE::SF_COHESION] = glm::normalize(cohesion.mDir[FCE::SF_COHESION]);
	}
}
void Alignment()
{
	auto ECS = FCE::Engine::GetRegistery();
	auto view = ECS->view<FCE::EnemyComponent, FCE::SteeringForce, FCE::Transform>();
	for (auto [handle, EnemyComp, allignment, transform] : view.each())
	{
		if (allignment.mFlags & FCE::DISABLE_ALLIGNMENT)
			continue;

		allignment.mDir[FCE::SF_ALLIGNMENT] = glm::vec3(0, 0, 0);
		if (EnemyComp.groupings == -1 || mGroups[EnemyComp.groupings].size() <= 1)
			continue;

		for (auto it : mGroups[EnemyComp.groupings])
		{
			if (it != handle)
			{
				auto vel = ECS->try_get<FCE::VelocityComponent>(it);
				if(vel)
				allignment.mDir[FCE::SF_ALLIGNMENT] += vel->mDir * vel->mSpeed;
			}
		}

		allignment.mDir[FCE::SF_ALLIGNMENT] /= static_cast<float>(mGroups[EnemyComp.groupings].size() - 1);
		auto vel = ECS->get<FCE::VelocityComponent>(handle);
		allignment.mDir[FCE::SF_ALLIGNMENT] -= vel.mDir * vel.mSpeed;

		allignment.mSpeed[FCE::SF_ALLIGNMENT] = glm::length(allignment.mDir[FCE::SF_ALLIGNMENT]);
		allignment.mDir[FCE::SF_ALLIGNMENT] = glm::normalize(allignment.mDir[FCE::SF_ALLIGNMENT]);
	}
}

bool WallAvoidanceRebound(glm::vec3& StartPos, glm::vec3& IncommingDir, glm::vec3 Normal, float length, int RecursionNumber);
void WallAvoidance(float DT)
{
	auto view = FCE::Engine::GetRegistery()->view<FCE::Transform, FCE::SteeringForce,FCE::VelocityComponent>();
	for (auto [handle, transform, wallavoindance,velocity] : view.each())
	{
		if (wallavoindance.mFlags & FCE::DISABLE_WALLAVOIDANCE || velocity.mSpeed < 0.1f)
			continue;

		
		if (wallavoindance.mDelay->IsOver())
		{
			wallavoindance.mDir[FCE::SF_WALLAVOIDANCE] = { 0,0,0 };
			wallavoindance.mSpeed[FCE::SF_WALLAVOIDANCE] = 0;

			wallavoindance.mDelay->Reset();
			

			//check first if you see something with the forward vector
			glm::vec3 Dir = glm::normalize(velocity.mDir * velocity.mSpeed);
			glm::vec3 start = transform.mTransform;
			float RayLength = (velocity.mSpeed / DT * wallavoindance.mDelay->mTime + wallavoindance.mDistToCheck);
			glm::vec3 end = start + Dir * RayLength;

			std::vector<FCE::RayCollisionData> data = FCE::CollisionSystem::ShootRay(start,end, gWallAvoindanceFlags);

			if (data.empty())
			{
				//else test the the slightly angled to the left/right vector
				//-10 degrees
				float theta = -0.7853981634f;
				FCE::Rotate2D(Dir, theta);
				end = start + Dir * RayLength;
				data = FCE::CollisionSystem::ShootRay(start, end, gWallAvoindanceFlags);

				if (data.empty())
				{
					//10 degree angle
					theta *= -2;
					FCE::Rotate2D(Dir, theta);
					end = start + Dir * RayLength;
					data = FCE::CollisionSystem::ShootRay(start, end, gWallAvoindanceFlags);
				}
			}

			//no walls found so do nothing
			if (!data.empty())
			{
				float PointDistance = data[0].mHitFraction;
				glm::vec3 Point = transform.mTransform + Dir * PointDistance * RayLength;
				Point = Point + data[0].mHitNormal * (1 - PointDistance);

				WallAvoidanceRebound(Point, Dir, data[0].mHitNormal,RayLength, 3);
				wallavoindance.mDir[FCE::SF_WALLAVOIDANCE] = Dir * (1 - PointDistance) * RayLength;
				if(FCE::Engine::GetDebugDrawing())
				FCE::Engine::GetRenderer()->DrawLine(transform.mTransform, transform.mTransform + wallavoindance.mDir[FCE::SF_WALLAVOIDANCE],glm::vec3(1,1,1),glm::vec3(1,1,1),true);

				wallavoindance.mSpeed[FCE::SF_WALLAVOIDANCE] = glm::length(wallavoindance.mDir[FCE::SF_WALLAVOIDANCE]);
				wallavoindance.mDir[FCE::SF_WALLAVOIDANCE] = glm::normalize(wallavoindance.mDir[FCE::SF_WALLAVOIDANCE]);
			}
		}
		else
		{
			if (FCE::Engine::GetDebugDrawing())
			FCE::Engine::GetRenderer()->DrawLine(transform.mTransform, transform.mTransform + wallavoindance.mDir[FCE::SF_WALLAVOIDANCE], glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), true);
			//wallavoindance.mSpeed[FCE::SF_WALLAVOIDANCE] = FCE::Lerp(wallavoindance.mSpeed[FCE::SF_WALLAVOIDANCE], 0, 0.1f);
		}
	}
}
//returns true it found a free spot
bool WallAvoidanceRebound(glm::vec3& StartPos,glm::vec3& IncommingDir,glm::vec3 Normal,float length, int RecursionNumber)
{
	RecursionNumber--;
	if (RecursionNumber < 0)
		return false;

	//change the direction based on the normal
	IncommingDir = FCE::Refract(IncommingDir, Normal);
	glm::vec3 End = StartPos + IncommingDir * length;

	//glm::vec3 End = StartPos + Normal * length;

	auto data = FCE::CollisionSystem::ShootRay(StartPos, End, gWallAvoindanceFlags);
	if (!data.empty())
	{
		StartPos += IncommingDir * data[0].mHitFraction * length;
		return WallAvoidanceRebound(StartPos,IncommingDir,data[0].mHitNormal,length,RecursionNumber);
		//return WallAvoidanceRebound(StartPos, IncommingDir, data[0].mHitNormal, length, RecursionNumber);
	}
	StartPos += IncommingDir * length;
	return true;


}

bool AccumulateForce(glm::vec3& CurrentForce,float maxSpeed, glm::vec3 ForceToAdd)
{
	if (glm::length2(ForceToAdd) < 0.1f)
		return true;

	float MagnitudeSoFar = length(CurrentForce);

	float MagnitudeRemaining = maxSpeed - MagnitudeSoFar;

	if (MagnitudeRemaining <= 0.01f) return false;

	float MagnitudeToAdd = glm::length(ForceToAdd);

	if (MagnitudeToAdd < MagnitudeRemaining)
	{
		CurrentForce += ForceToAdd;
	}
	else
	{
		//add it to the steering force
		CurrentForce += (normalize(ForceToAdd) * MagnitudeRemaining);
		return false;
	}
	return true;
}

void Steer(glm::vec3& MovementForce,glm::vec3 newForce,float maxSpeed,float mass)
{
	if (glm::length2(newForce) < 0.1f)
		return;

	glm::vec3 steering = newForce - MovementForce;

	if (glm::length2(steering) > maxSpeed * maxSpeed)
		steering = normalize(steering) * maxSpeed;

	steering = (steering / mass);
	MovementForce = MovementForce + steering;

	if (glm::length2(MovementForce) > maxSpeed * maxSpeed)
		MovementForce = normalize(MovementForce) * maxSpeed;
}

bool ShouldWait(entt::entity ent)
{
	auto ECS = FCE::Engine::GetRegistery();
	auto waitComponent = ECS->try_get<FCE::MovementLock>(ent);

	if (waitComponent)
	{
		auto transform = ECS->try_get<FCE::Transform>(ent);
		auto otherTransform = ECS->try_get<FCE::Transform>(waitComponent->mTarget);
		if (!transform || !otherTransform)
			return false;

		if (waitComponent->mLocked == true)
			if (glm::distance2(transform->mTransform, otherTransform->mTransform) < waitComponent->mDist * waitComponent->mDist)
				waitComponent->mLocked = false;
		else
			if (glm::distance2(transform->mTransform, otherTransform->mTransform) > waitComponent->mDist * waitComponent->mDist)
				waitComponent->mLocked = true;

		return waitComponent->mLocked;

	}
	return false;
}

void DestroySteeringForce(entt::registry& reg, entt::entity e)
{
	auto D = reg.get<FCE::SteeringForce>(e);
	FCE::TimeHandler::DeleteTimer(D.mDelay);
};

void FCE::EnemySystem::Init(float Interval, uint32_t wallAvoidanceFlags)
{

	gInterval = TimeHandler::CreateTimer(Interval);
	gWallAvoindanceFlags = wallAvoidanceFlags;
	FCE::Engine::GetRegistery()->on_destroy<SteeringForce>().connect<DestroySteeringForce>();
}

void FCE::EnemySystem::Update(float DT)
{

	if (gInterval->IsOver())
	{
		UpdateGroups();
		gInterval->Reset();
		Seperation();
		Cohesion();
		Alignment();
	}
	WallAvoidance(DT);

	for (auto [handle, enemyComp] : FCE::Engine::GetRegistery()->view<FCE::EnemyComponent>().each())
	{
		if (ShouldWait(handle))
			continue;


		auto steeringforce = FCE::Engine::GetRegistery()->try_get<FCE::SteeringForce>(handle);
		auto& velocity = FCE::Engine::GetRegistery()->get_or_emplace<FCE::VelocityComponent>(handle);
		glm::vec3 EnemyMovement = enemyComp.mMovementFunction(DT,handle);
		if (!steeringforce)
		{
			if (glm::length2(EnemyMovement) > 0.01f)
			{
				velocity.mDir = glm::normalize(EnemyMovement);
				velocity.mSpeed = glm::length(EnemyMovement);
			}
			continue;
		}
		glm::vec3 vel = velocity.mDir* velocity.mSpeed;
		steeringforce->mDir[SF_SMOOTHING] = vel * steeringforce->mPriority[SF_SMOOTHING];


		glm::vec3 SteeringForce{ 0,0,0};
		if (AccumulateForce(SteeringForce,steeringforce->mMaxSpeed, steeringforce->mDir[SF_SMOOTHING]))
			if (AccumulateForce(SteeringForce, steeringforce->mMaxSpeed, steeringforce->mDir[SF_WALLAVOIDANCE] * steeringforce->mSpeed[SF_WALLAVOIDANCE] * steeringforce->mPriority[SF_WALLAVOIDANCE]))
				if (AccumulateForce(SteeringForce, steeringforce->mMaxSpeed, steeringforce->mDir[SF_SEPARATION] * steeringforce->mSpeed[SF_SEPARATION] * steeringforce->mPriority[SF_SEPARATION]))
					if (AccumulateForce(SteeringForce, steeringforce->mMaxSpeed, EnemyMovement))
						if (AccumulateForce(SteeringForce, steeringforce->mMaxSpeed, steeringforce->mDir[SF_COHESION] * steeringforce->mSpeed[SF_COHESION] * steeringforce->mPriority[SF_COHESION]))
							AccumulateForce(SteeringForce, steeringforce->mMaxSpeed, steeringforce->mDir[SF_ALLIGNMENT] * steeringforce->mSpeed[SF_ALLIGNMENT] * steeringforce->mPriority[SF_ALLIGNMENT]);
		
		Steer(vel,SteeringForce,steeringforce->mMaxSpeed,1);
		velocity.mSpeed = glm::length(vel);
		if(velocity.mSpeed > 0.1f)
		velocity.mDir = glm::normalize(vel);
	}
	
}

void FCE::EnemySystem::ForceUpdateGroupings()
{
	UpdateGroups();
}

void FCE::EnemySystem::UpdateGroups()
{
	auto ECS = FCE::Engine::GetRegistery();
	mGroups.clear();
	{
		auto view = ECS->view<FCE::EnemyComponent>();
		for (auto [handle, enemyComp] : view.each())
		{
			enemyComp.groupings = -1;
		}
	}
	auto view = ECS->view<FCE::EnemyComponent, FCE::Transform>();
	for (auto [handle, enemyComp,transform] : view.each())
	{
		if (enemyComp.groupings != -1 || !(ECS->any_of<FCE::SteeringForce>(handle)))
			continue;

		auto Jview = ECS->view<FCE::EnemyComponent, FCE::Transform>();
		for (auto [otherHandle, otherEnemyComp, otherTransform] : Jview.each())
		{
			if (otherHandle == handle || !(ECS->any_of<FCE::SteeringForce>(otherHandle)))
				continue;
			if (enemyComp.groupings != -1 && otherEnemyComp.groupings != -1)
				continue;

			glm::vec3 to = otherTransform.mTransform - transform.mTransform;
			float range = otherEnemyComp.groupingRadius + enemyComp.groupingRadius;

			if ((glm::length2(to) < range * range))
			{
				if (enemyComp.groupings != -1 && otherEnemyComp.groupings != -1)
				{
					int group = mGroups.size();
					enemyComp.groupings = group;
					otherEnemyComp.groupings = group;
					std::vector<entt::entity> list;
					list.push_back(handle);
					list.push_back(otherHandle);
					mGroups.push_back(list);
				}
				else if (enemyComp.groupings != -1)
				{
					otherEnemyComp.groupings = enemyComp.groupings;
					mGroups[otherEnemyComp.groupings].push_back(otherHandle);
				}
				else if (otherEnemyComp.groupings != -1)
				{
					enemyComp.groupings = otherEnemyComp.groupings;
					mGroups[enemyComp.groupings].push_back(handle);
				}
			}
		}
	}
}
