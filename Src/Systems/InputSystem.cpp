#include "Core/FCEpch.h"
#include "Systems/InputSystem.h"
#include "Core/Engine.h"
#include "Components/InputComponent.h"
void FCE::InputSystem::AddComponent(entt::entity ent, std::function<void(entt::entity,float)> callback)
{
	FCE::Engine::GetRegistery()->emplace<InputComponent>(ent, callback);
}

void FCE::InputSystem::Update(float DT)
{
	auto view = FCE::Engine::GetRegistery()->view<InputComponent>();
	for (auto [entity, input] : view.each())
	{
		input.mCallback(entity, DT);
	}
}
