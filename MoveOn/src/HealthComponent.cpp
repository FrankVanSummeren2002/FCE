#include "PCH.h"
#include "HealthComponent.h"
#include "Systems/CollisionSystem.h"
#include "Maps/Level.h"
#include "Maps/World.h"
#include "LoadingLevel.h"
#include "WeaponComponent.h"
void DefaultDeathFunc(entt::entity self) 
{
	auto weapon = FCE::Engine::GetRegistery()->try_get<WeaponOwner>(self);
	if (weapon)
		DefaultDeathFunc(weapon->mWeapon);

	FCE::Engine::GetActiveWorld()->GetLevel()->RemoveEntityFromDeletionQueue(self);
	FCE::Engine::GetRegistery()->destroy(self);
};

HealthComponent::HealthComponent(float health) : mHealth{ health }, mMaxHealth{ health }
{
	mDeathFunction = DefaultDeathFunc;
}

HealthComponent::HealthComponent(float health, std::function<void(entt::entity)> DeathFunction) : mHealth{ health }, mMaxHealth{ health }
{
	mDeathFunction = DeathFunction;
}
