#pragma once
class WeaponSystem
{
public:
	static void Init();
	static void Shoot(entt::entity Self);
	static void Update(float DT);

};