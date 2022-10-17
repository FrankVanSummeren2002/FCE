#include "PCH.h"
#include "WeaponComponent.h"


void RotateAround2D::Rotate(glm::vec2 Pos, glm::vec2 OtherPos, entt::entity self)
{
	mDir = glm::normalize(OtherPos - Pos);
	auto transform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(self);
	auto otherTransform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(mTarget);
	if (transform && otherTransform)
	{
		if (mRotateToDir)
		{
			float dot = glm::dot(mDir, glm::vec2(0, -1));
			float angle = acos(dot);

			if (Pos.x > OtherPos.x)
				angle *= -1;

			transform->mRotation2D = angle + mRotationOffset;
			//transform->mRotation = mRotationOffset3D * glm::rotate(glm::mat4(1),angle - glm::radians(90.f),glm::vec3(0,0,1));
		}
		transform->mTransform = otherTransform->mTransform + glm::vec3(mDir * mDistance,otherTransform->mTransform.z);
		transform->mTransform.z = 0;
	}
}


