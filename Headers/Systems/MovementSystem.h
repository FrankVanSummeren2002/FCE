#pragma once
namespace FCE
{
	class MovementSystem
	{
	public:
	
		static void UpdateMovement(float DT);
		static void AddImpulse(entt::entity ent, glm::vec2 impulse);
	};
}