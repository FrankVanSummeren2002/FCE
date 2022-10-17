#pragma once
#include "Collision/CollisionTable.h"
class btCollisionWorld;


namespace FCE
{
	struct CollisionComponent;
	struct RayCollisionData
	{
		entt::entity mEntity = entt::null;
		glm::vec3 mHitNormal = { 0, 0, 0 };
		float mHitFraction = 0.f;
	};


	class CollisionSystem
	{
		static bool OnGround(FCE::CollisionComponent* comp, glm::vec3 start, glm::vec3 end);
	public:
		static void SetCollisionTable(FCE::CollisionMaskTable table);
		static FCE::CollisionMaskTable GetCollisionTable();
		static void Init(glm::vec3 Gravity);
		static void Update(float DT);
		//uses the transform component to change collision components location
		static void UpdatePosition(entt::entity);
		static void AddComponent(entt::entity Entity, glm::vec3 Size, glm::vec3 Offset, float Mass, int group = 0,
			std::function<void(entt::entity self,entt::entity other)> Callback = {});
		static void CleanUp();
		static bool OnGround(entt::entity ent, glm::vec3 offset);
		static std::vector<RayCollisionData> ShootRay(glm::vec3 Start, glm::vec3 End, uint32_t flags);
		static void DeleteComponent(entt::registry& reg, entt::entity ent);
		static void SetDebugDraw(bool debug);
	};
}

