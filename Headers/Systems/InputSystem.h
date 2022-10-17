#pragma once
namespace FCE
{
	class InputSystem
	{
	public:
		static void AddComponent(entt::entity, std::function<void(entt::entity,float)> callback);
		static void Update(float DT);
	};
}

