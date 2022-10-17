#include "PCH.h"
#include "EnemyFunction.h"
#include "LoadingLevel.h"
#include "WeaponComponent.h"
#include "Components/MovementComponent.h"
#include "WeaponSystem.h"
#include "AStar.h"
#include "Systems/CollisionSystem.h"

void NextPoint(FCE::AStarComponent* comp)
{
	FCE::PathPoint* p = comp->mPath;
	comp->mPath = comp->mPath->mNext;
	delete p;
}

void ClearPathFinding(FCE::AStarComponent* comp)
{
	while (comp->mPath)
		NextPoint(comp);
}
void UpdateToNextPoint(FCE::AStarComponent* comp,glm::vec3 position)
{
	glm::vec3 DirToPoint = comp->mPath->mPoint->mPosition - position;
	float Dist = glm::length(DirToPoint);
	DirToPoint = glm::normalize(DirToPoint);
	if (Dist < comp->mDistanceToPoint)
		NextPoint(comp);
}
bool ShouldUsePathFinding(FCE::AStarComponent* comp, glm::vec3 start, glm::vec3 direction)
{
	//if it does not have the a star component then dont use it
	if (!comp)
		return false;
	auto data = FCE::CollisionSystem::ShootRay(start, start + direction * comp->mPlayerCheckDistance, COLLISION_TYPE_PLAYER | COLLISION_TYPE_WALL);
	//player outside of detection range so use astar
	if (data.empty())
		return true;
	//first thing it hit was the player so dont use path finding
	if (FCE::Engine::GetRegistery()->try_get<FCE::PlayerComponent>(data[0].mEntity))
		return false;
	//it hit a wall so use something else
	return true;
}

glm::vec3 DefaultEnemyFunction(float DT,entt::entity self)
{
	auto ECS = FCE::Engine::GetRegistery();
	const auto& transform = ECS->get<FCE::Transform>(self);
	const auto& PlayerTransform = ECS->get<FCE::Transform>(GetPlayer());

	//shooting
	auto weaponOwner = ECS->try_get<WeaponOwner>(self);
	if (weaponOwner)
	{
		auto weapon = ECS->try_get<WeaponComponent>(weaponOwner->mWeapon);
		auto waitComp = ECS->try_get<FCE::MovementLock>(weaponOwner->mWeapon);
		if (weapon)
		{
			auto Rot = ECS->try_get<RotateAround2D>(weaponOwner->mWeapon);
			if (Rot)
				Rot->Rotate(transform.mTransform,PlayerTransform.mTransform,weaponOwner->mWeapon);
			bool canShoot = true;
			if (waitComp)
				canShoot = !waitComp->mLocked;
			if (canShoot)
				WeaponSystem::Shoot(weaponOwner->mWeapon);
		}
	}
	
	auto PathFinding = ECS->try_get<FCE::AStarComponent>(self);
	if (PathFinding)
	{
		if (PathFinding->mInterval->IsOver())
		{
			PathFinding->mInterval->Reset();
			ClearPathFinding(PathFinding);
			PathFinding->mPath =  FCE::AStarSystem::FindPath(
				FCE::AStarSystem::ConvertWorldToGridPosition(transform.mTransform),
				FCE::AStarSystem::ConvertWorldToGridPosition(PlayerTransform.mTransform));
		}
	}
	//movement
	glm::vec3 result(0);

	glm::vec3 Dir = PlayerTransform.mTransform - transform.mTransform;
	float length = glm::length(Dir);
	if (length > 0.1f)
	{
		Dir = glm::normalize(Dir);

		if (ShouldUsePathFinding(PathFinding, transform.mTransform, Dir))
		{
			// if the path has reached the end try and find a new one
			if(!PathFinding->mPath)
				PathFinding->mPath = FCE::AStarSystem::FindPath(
					FCE::AStarSystem::ConvertWorldToGridPosition(transform.mTransform),
					FCE::AStarSystem::ConvertWorldToGridPosition(PlayerTransform.mTransform));
			else
			{
				//check if we can go to the next point
				UpdateToNextPoint(PathFinding, transform.mTransform);
				//move towards the direction
				if (PathFinding->mPath)
				{
					glm::vec3 PathDir = glm::normalize(PathFinding->mPath->mPoint->mPosition - transform.mTransform);
					result = PathDir * 20.f;
				}
			}

		}
		else
		{
			auto random = ECS->try_get<RandomMovement>(self);

			if (!random)
			{
				random = &ECS->emplace<RandomMovement>(self);
				random->mInterval = FCE::TimeHandler::CreateTimer(2);
			}
			if (random->mInterval->IsOver())
				random->strength = (rand() % 2 * 2 - 1) * 10.f + (float)(rand() % 20);

			random->Vel = glm::cross(Dir, glm::vec3(0, 0, 1)) * random->strength;

			if (length < 64)
				Dir *= -1;
			else if (length < 92)
				Dir *= 0;
			result = random->Vel + Dir * 40.f;
			if (glm::length2(result) > 0.1f)
				result = glm::normalize(result) * 20.f;
		}
	}
	return result;
}


