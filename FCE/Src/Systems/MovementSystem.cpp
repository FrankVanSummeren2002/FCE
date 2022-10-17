#include "Core/FCEpch.h"
#include "Systems/MovementSystem.h"
#include "Core/Engine.h"
#include "Components/MovementComponent.h"
#include "Components/CommonComponents.h"
#include "Systems/CollisionSystem.h"

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"

#include "Components/EnemyComponent.h"

#include "Core/Core.h"

struct Impulse2D
{
	glm::vec2 mImpulse;
	Impulse2D(glm::vec2 impulse) :mImpulse{ impulse } {};
};


void FCE::MovementSystem::UpdateMovement(float DT)
{
	{
		auto view = FCE::Engine::GetRegistery()->view<FCE::VelocityComponent>();
		for (auto [entity, velocity] : view.each())
		{
			if (glm::length2(velocity.mDir) > 0.1f && velocity.mSpeed > 0.1f)
			{
				auto transform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(entity);
				auto EnemyComp = FCE::Engine::GetRegistery()->try_get<FCE::EnemyComponent>(entity);
				auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(entity);
				if (coll)
				{
					if (velocity.mPhysicsBased)
					{
						coll->mBody->activate();
						coll->mBody->applyCentralForce(glmToBullet(velocity.mSpeed * glm::normalize(velocity.mDir) * DT));
					}
					else
					{
						transform->mTransform += velocity.mDir * velocity.mSpeed * DT;
						FCE::CollisionSystem::UpdatePosition(entity);
					}
				}
			}
		}
	}

	{
		auto view = FCE::Engine::GetRegistery()->view<Impulse2D>();
		for (auto [entity, impulse] : view.each())
		{
			if (glm::length2(impulse.mImpulse) > 0.1f)
			{
				auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(entity);
				if (coll)
				{
					coll->mBody->applyCentralImpulse(glmToBullet(glm::vec3(impulse.mImpulse, 0)));
					coll->mBody->activate();
				}

				FCE::Engine::GetRegistery()->remove<Impulse2D>(entity);
			}
		}
	}
	
}

void FCE::MovementSystem::AddImpulse(entt::entity ent,glm::vec2 impulse)
{
	auto vel = FCE::Engine::GetRegistery()->try_get<Impulse2D>(ent);
	if (vel)
		vel->mImpulse += impulse;
	else
		FCE::Engine::GetRegistery()->emplace<Impulse2D>(ent,impulse);
}
