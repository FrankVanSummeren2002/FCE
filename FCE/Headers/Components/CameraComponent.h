#pragma once
namespace FCE
{
	struct Camera3DComponent
	{
		glm::vec3 mPosition{};
		glm::vec3 mLookAtDir{};
		glm::vec3 mUp{};
		glm::mat4 mProjMatrix{};
		Camera3DComponent() {};
		Camera3DComponent(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up, glm::mat4 projection)
			: mPosition{ position }, mLookAtDir{ lookDir }, mUp{ up }, mProjMatrix{ projection } {};
	};

	struct Camera2DComponent
	{
		glm::vec4 mPosition{};
		Camera2DComponent() {};
		Camera2DComponent(glm::vec4 position) :mPosition{ position } {};
	};

	struct CameraFollowComponent
	{
		entt::entity mFollowing{};
		float mAlpha = 0;
		bool mBounds = false;
		bool mLookAt{false};
		glm::vec3 mOffset{};
		glm::vec3 mMinBounds{};
		glm::vec3 mMaxbounds{};
		CameraFollowComponent() {}
		CameraFollowComponent(entt::entity Follow, float Alpha, bool LookAt, glm::vec3 offset = glm::vec3{ 0.f }, bool bounds = false, glm::vec3 MinBounds = glm::vec3(1), glm::vec3 MaxBounds = glm::vec3(1))
			: mFollowing{ Follow }, mAlpha{ Alpha }, mBounds{ bounds }, mMinBounds{ MinBounds }, mMaxbounds{ MaxBounds }, mLookAt{LookAt}, mOffset{ offset }{};
	};
}