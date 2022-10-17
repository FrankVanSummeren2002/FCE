#include "PCH.h"
#include "HealthSystem.h"
#include "HealthComponent.h"
void HealthSystem::Update()
{
	auto ECS = FCE::Engine::GetRegistery();
	auto view = ECS->view<DamagedComponent>();

	for (auto [Handle, Damage] : view.each())
	{
		auto health = ECS->try_get<HealthComponent>(Handle);
		if (health)
			health->mHealth -= Damage.mDamage;

		ECS->remove<DamagedComponent>(Handle);

		if (health)
			if (health->mHealth <= 0)
				health->mDeathFunction(Handle);
	}
}
