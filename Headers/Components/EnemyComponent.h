#pragma once
namespace FCE
{
	struct EnemyComponent
	{
		int groupings = -1;
		float groupingRadius;
		std::function<glm::vec3(float DT,entt::entity)> mMovementFunction;
		EnemyComponent(float GroupingRadius, std::function<glm::vec3(float DT,entt::entity self)> movementFunction)
			: groupingRadius{ GroupingRadius }, mMovementFunction{movementFunction}{};
	};
}