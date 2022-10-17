#include "Core/FCEpch.h"
#include "Systems/CameraSystem.h"
#include "Core/Engine.h"
#include "Components/CommonComponents.h"
#include "Components/CameraComponent.h"
#include "Core/Core.h"
void FCE::CameraSystem::Add2DComponent(entt::entity ent,glm::vec4 Pos)
{
	FCE::Engine::GetRegistery()->emplace<FCE::Camera2DComponent>(ent, Pos);
	if (!FCE::Engine::GetRenderMode())
	{
		if (!FCE::Engine::GetRegistery()->any_of<FCE::Camera2DComponent>(FCE::Engine::GetActiveCameraHandle()))
			FCE::Engine::SetActiveCameraHandle(ent);
	}

}

void FCE::CameraSystem::Add3DComponent(entt::entity ent, glm::vec3 Pos, glm::vec3 LookDir, glm::vec3 up, glm::mat4 projection)
{
	FCE::Engine::GetRegistery()->emplace<FCE::Camera3DComponent>(ent,Pos,LookDir,up,projection);
	if (FCE::Engine::GetRenderMode())
	{
		if (!FCE::Engine::GetRegistery()->any_of<FCE::Camera3DComponent>(FCE::Engine::GetActiveCameraHandle()))
			FCE::Engine::SetActiveCameraHandle(ent);
	}
}

void FCE::CameraSystem::AddFollowComponent(entt::entity ent, entt::entity following, float lerp,bool lookAt,glm::vec3 Offset, bool Bounds, glm::vec3 min, glm::vec3 max )
{
	MinMaxVector(min, max);
	auto FollowComponent = FCE::Engine::GetRegistery()->try_get<FCE::CameraFollowComponent>(ent);
	if (FollowComponent)
	{
		FollowComponent->mAlpha = lerp;
		FollowComponent->mBounds = Bounds;
		FollowComponent->mFollowing = following;
		FollowComponent->mMaxbounds = max;
		FollowComponent->mMinBounds = min;
		FollowComponent->mLookAt = lookAt;
		FollowComponent->mOffset = Offset;
	}
	else
	{
		FCE::Engine::GetRegistery()->emplace<FCE::CameraFollowComponent>(ent, following, lerp,lookAt, Offset,Bounds,min,max);
	}

}

void FCE::CameraSystem::Update(float DT)
{
	entt::registry* reg = FCE::Engine::GetRegistery();
	auto view = reg->view<CameraFollowComponent>();


	for (auto [Entity, Follow] : view.each())
	{
		auto CameraTransform = reg->try_get<FCE::Camera2DComponent>(Entity);
		auto Camera3DTransform = reg->try_get<FCE::Camera3DComponent>(Entity);
		auto FollowTransfrom = reg->try_get<FCE::Transform>(Follow.mFollowing);
		
		float val = CameraTransform->mPosition.w;
		//2D
		if (CameraTransform && FollowTransfrom)
		{

			CameraTransform->mPosition = CameraTransform->mPosition + (glm::vec4(FollowTransfrom->mTransform,val) - CameraTransform->mPosition) * Follow.mAlpha;
			if (Follow.mBounds)
			{
				CameraTransform->mPosition = glm::clamp(CameraTransform->mPosition, glm::vec4(Follow.mMinBounds, val), glm::vec4(Follow.mMaxbounds, val));
			}
		}
		//3D
		if (Camera3DTransform && FollowTransfrom)
		{
			//Camera3DTransform->mPosition = Camera3DTransform->mPosition + dir;
			Camera3DTransform->mPosition += ( FollowTransfrom->mTransform - Follow.mOffset - Camera3DTransform->mPosition);

			if (Follow.mLookAt)
				Camera3DTransform->mLookAtDir = glm::normalize(FollowTransfrom->mTransform - Camera3DTransform->mPosition);

			if (Follow.mBounds)
				Camera3DTransform->mPosition = glm::clamp(Camera3DTransform->mPosition, Follow.mMinBounds, Follow.mMaxbounds);
		}


	}
}
#include "Header/VulkanFrontEnd.h"
#include "UI/ImGuiHandler.h"
glm::vec2 FCE::CameraSystem::Convert2DPosToScreenSpace(entt::entity camera, glm::vec2 Pos)
{
	auto cam = FCE::Engine::GetRegistery()->try_get<Camera2DComponent>(camera);
	glm::vec2 pos = Pos - glm::vec2(cam->mPosition);
	glm::vec2 min, max;
	ImGuiHandler::GetMainWindowMinMax(min, max);
	pos = (pos / glm::vec2(1700, 900)) * FCE::Engine::GetRenderer()->GetScreenSize();
	return pos;
}