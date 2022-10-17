#include "PCH.h"

#include "LoadingLevel.h"

#include "Maps/World.h"
#include"Maps/Level.h"

#include "Systems/RenderSystem.h"
#include "Systems/CollisionSystem.h"
#include "WeaponSystem.h"

#include "Components/MovementComponent.h"
#include "WeaponComponent.h"
#include "HealthComponent.h"

void BulletDamage(entt::entity self, entt::entity other)
{
	auto health = FCE::Engine::GetRegistery()->try_get<HealthComponent>(other);
	auto bullet = FCE::Engine::GetRegistery()->try_get<BulletComp>(self);
	auto OtherColl = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(other);

	if (bullet)
	{
		bullet->death = true;
		if (health)
		{
			auto& damage = FCE::Engine::GetRegistery()->get_or_emplace<DamagedComponent>(other);
			damage.mDamage += bullet->mDamage;
		}
	}

}

void OnDestroyBullet(entt::registry& reg, entt::entity e)
{
	FCE::Engine::GetActiveWorld()->GetLevel()->RemoveEntityFromDeletionQueue(e);
	auto D = FCE::Engine::GetRegistery()->get<BulletComp>(e);
	FCE::TimeHandler::DeleteTimer(D.DeathTimer);
}

void OnDestroyWeapon(entt::registry& reg, entt::entity e)
{
	auto D = FCE::Engine::GetRegistery()->get<WeaponComponent>(e);
	FCE::TimeHandler::DeleteTimer(D.mCooldown);
}

void WeaponSystem::Init()
{
	FCE::Engine::GetRegistery()->on_destroy<BulletComp>().connect<OnDestroyBullet>();
	FCE::Engine::GetRegistery()->on_destroy<WeaponComponent>().connect<OnDestroyWeapon>();
}

void WeaponSystem::Shoot(entt::entity self)
{
	auto ECS = FCE::Engine::GetRegistery();
	auto Rot = ECS->try_get<RotateAround2D>(self);
	auto Weapon = ECS->try_get<WeaponComponent>(self);
	if (Rot && Weapon)
	{
		if (!Weapon->mCooldown->IsOver())
			return;

		Weapon->mCooldown->Reset();
		//create the bullet
		auto bulletHandle = ECS->create();
		//make sure the bullet dies when the level dies
		FCE::Engine::GetActiveWorld()->GetLevel()->AddEntityToDeletionQueue(bulletHandle);

		uint32_t bulletSize = 16;

		//transform
		auto& trans = ECS->emplace<FCE::Transform>(bulletHandle);
		trans = ECS->get<FCE::Transform>(self);
		trans.mTransform += glm::vec3(Rot->mDir, 0) * (float)bulletSize;
		trans.mSize = glm::vec3(3);

		//rendering
		FCE::RenderSystem::Add3DComponent(bulletHandle, "Models/wooden_sphere.obj", "textures/wood_Mat_BaseColor.png",
			"Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv",glm::vec3(0,10,0));

		//FCE::RenderSystem::Add2DComponentSingleTexture(bulletHandle, bulletSize, bulletSize, glm::vec2(1,1),
		//	"textures/kenney_pixelplatformer/Characters/character_0001.png",
		//	"Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");


		FCE::RenderSystem::Add2DComponentSpriteSheet(bulletHandle, bulletSize, bulletSize, glm::vec2{ 18,18 } * glm::vec2(11,7), glm::vec2{ 360,162 }, glm::vec2{ 18,18 }, glm::vec2(1, 1),
			"textures/kenney_pixelplatformer/Tilemap/tiles_packed.png", "Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");

		//collision
		FCE::CollisionSystem::AddComponent(bulletHandle, glm::vec3(bulletSize * 0.5f, bulletSize * 0.5f, 5),
			glm::vec3(0.f, 0.f, 0.f), 85, Weapon->mBulletCollisionType,BulletDamage);
		auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(bulletHandle);
		coll->mBody->setFlags(DISABLE_DEACTIVATION);
		coll->mBody->setGravity(btVector3(btScalar(0), btScalar(0), btScalar(0)));

		//velocity
		FCE::Engine::GetRegistery()->emplace<FCE::VelocityComponent>(bulletHandle, 0.5f, Weapon->mBulletSpeed, glm::vec3(Rot->mDir,0), false);

		//Bullet
		FCE::Engine::GetRegistery()->emplace<BulletComp>(bulletHandle,Weapon->mDamage);

	}
}

void WeaponSystem::Update(float DT)
{
	auto bullets = FCE::Engine::GetRegistery()->view<BulletComp>();
	for (auto [Handle, bullet] : bullets.each())
	{
		 
		if (bullet.death || bullet.DeathTimer->IsOver())
		{
			FCE::TimeHandler::DeleteTimer(bullet.DeathTimer);
			FCE::Engine::GetActiveWorld()->GetLevel()->RemoveEntityFromDeletionQueue(Handle);
			FCE::Engine::GetRegistery()->destroy(Handle);
		}
	}
}
