#pragma once
struct HealthComponent
{
	float mHealth{0};
	float mMaxHealth{0};
	std::function<void(entt::entity)> mDeathFunction;
	HealthComponent(float health);
	HealthComponent(float health,std::function<void(entt::entity)> DeathFunction);
};

struct DamagedComponent
{
	float mDamage = 0;
	DamagedComponent() {};
	DamagedComponent(float Damage) :mDamage{ Damage } {};
};

