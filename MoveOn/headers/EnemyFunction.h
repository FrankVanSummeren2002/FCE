#pragma once

namespace FCE
{
	struct PathPoint;
	struct AStarComponent;
}

struct RandomMovement
{
	glm::vec3 Vel{};
	float strength{0};
	FCE::Timer* mInterval = nullptr;
};



void ClearPathFinding(FCE::AStarComponent* comp);
glm::vec3 DefaultEnemyFunction(float DT,entt::entity self);