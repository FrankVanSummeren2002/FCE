#pragma once

struct RotateAround2D
{
	entt::entity mTarget{};
	glm::vec2 mDir{ 1,1 };
	float mDistance{0};
	bool mRotateToDir{false};
	float mRotationOffset{0};
	glm::mat4 mRotationOffset3D;
	RotateAround2D(entt::entity Target, float Distance, bool RotateToDir, float RotationOffset,glm::mat4 m3DRotationOffset = glm::mat4(1))
		:mTarget{ Target }, mDistance{ Distance }, mRotateToDir{ RotateToDir }, mRotationOffset{ RotationOffset }, mRotationOffset3D{m3DRotationOffset}{};
	void Rotate(glm::vec2 Pos, glm::vec2 OtherPos, entt::entity self);
};
struct WeaponOwner
{
	entt::entity mWeapon;
	WeaponOwner(entt::entity Weapon) : mWeapon{ Weapon } {};
};
struct WeaponComponent
{
	FCE::Timer* mCooldown = nullptr;
	float mDamage = 0;
	float mBulletSpeed = 0;
	uint32_t mBulletCollisionType{ 0 };
	WeaponComponent() {};
	WeaponComponent(float Damage, float Cooldown,float BulletSpeed, uint32_t BulletCollisionType): mDamage{ Damage }
	, mBulletCollisionType{ BulletCollisionType }, mBulletSpeed{ BulletSpeed } { mCooldown = FCE::TimeHandler::CreateTimer(Cooldown); };

	~WeaponComponent() { FCE::TimeHandler::DeleteTimer(mCooldown); }

};

struct BulletComp
{
	FCE::Timer* DeathTimer{ FCE::TimeHandler::CreateTimer(1000) };;
	float mDamage;
	bool death{false};
	BulletComp(float Damage) : mDamage{ Damage } {};
	~BulletComp() {};
};

