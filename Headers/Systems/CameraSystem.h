#pragma once
namespace FCE
{
	class CameraSystem
	{
	public:
		//if there is no active camera and the render mode is 2d then it will automaticly update the
		//Engines active camera. Otherwise you have to do it trough Engine::SetActiveCameraHandle
		static void Add2DComponent(entt::entity ent,glm::vec4 Pos);
		//if there is no active camera and the render mode is 3d then it will automaticly update the
		//Engines active camera. Otherwise you have to do it trough Engine::SetActiveCameraHandle
		static void Add3DComponent(entt::entity ent,glm::vec3 Pos, glm::vec3 LookDir, glm::vec3 up, glm::mat4 projection);
		static void AddFollowComponent(entt::entity ent, entt::entity following, float lerp,bool lookAt = false, glm::vec3 offset = glm::vec3(0.0f),bool Bounds = false, glm::vec3 min = glm::vec3(1) ,glm::vec3 max = glm::vec3(1));
		static void Update(float DT);
		static glm::vec2 Convert2DPosToScreenSpace(entt::entity camera, glm::vec2 Pos);
	};
}

