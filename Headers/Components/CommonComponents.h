#pragma once
#include "Header/Handles.h"

class btCollisionShape;
class btRigidBody;
namespace FCE
{
	struct Transform
	{
		glm::vec3 mTransform{};
		glm::vec3 mSize{};
		glm::vec2 mSize2D{};
		glm::mat4 mRotation{};
		float mRotation2D = 0;
		Transform() {};
		Transform(glm::vec3 transform, glm::vec3 Size,glm::vec2 Size2D, glm::mat4 Rot,float Rotation2D = 0);
	};



	struct Render3DComponent
	{
		FVR::MeshHandle mMeshHandle;
		FVR::TextureHandle mTextureHandle;
		bool mDisabled = false;
		glm::vec3 mOffset;
		Render3DComponent(FVR::MeshHandle meshHandle, FVR::TextureHandle textureHandle, glm::vec3 Offset)
		{
			mOffset = Offset;
			mMeshHandle = meshHandle;
			mTextureHandle = textureHandle;
		}
	};

	struct Render2DComponent
	{
		FVR::QuadHandle mQuadHandle;
		FVR::TextureHandle mTextureHandle;
		glm::vec2 mRepetition{1};
		glm::vec2 mSize;
		bool mDisabled = false;
		Render2DComponent(FVR::QuadHandle quadHandle, FVR::TextureHandle textureHandle,glm::vec2 size, glm::vec2 Repetition = glm::vec2(1))
		{
			mQuadHandle = quadHandle;
			mTextureHandle = textureHandle;
			mRepetition = Repetition;
			mSize = size;
		}
	};
	//only construct this after you construct a transform 
	//uses the transforms size for its Size
	struct CollisionComponent
	{
		std::function<void(entt::entity self,entt::entity OtherEntity)> mCollisionCallback;
		glm::vec3 offset;
		int collisionGroup;
		uint32_t mCollisionChannels;
		btCollisionShape* mShape;
		btRigidBody* mBody;
		//only construct this after you construct a transform 
		CollisionComponent(entt::entity Entity, glm::vec3 Size, glm::vec3 Offset, float Mass, int CollisionGroup,uint32_t collisionChannels, std::function<void(entt::entity self, entt::entity OtherEntity)> collisionFunction);
	};
	//is simply a notifier to the right entity
	//does not contain anything
	struct PlayerComponent
	{
		entt::entity self;
		PlayerComponent(entt::entity ref) :self{ref} {};
	};
}