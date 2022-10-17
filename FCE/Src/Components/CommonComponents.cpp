#include "Core/FCEpch.h"
#include "Components/CommonComponents.h"
#include "Core/Engine.h"
#include "btBulletDynamicsCommon.h"
#include "gtx/matrix_decompose.hpp"
FCE::CollisionComponent::CollisionComponent(entt::entity Entity,glm::vec3 Size,glm::vec3 Offset,float Mass,int CollisionGroup, uint32_t collisionChannels, std::function<void(entt::entity self, entt::entity OtherEntity)> collisionFunction)
{
	offset = Offset;
	collisionGroup = CollisionGroup;
	mCollisionChannels = collisionChannels;
	mCollisionCallback = collisionFunction;
		mShape = new btBoxShape(btVector3(btScalar(Size.x), btScalar(Size.y), btScalar(Size.z)));
		mShape->setLocalScaling(btVector3(btScalar(1), btScalar(1), btScalar(1)));
		btTransform btTrans;
		btTrans.setIdentity();

		auto it = FCE::Engine::GetRegistery()->try_get<Transform>(Entity);
		if (it)
			btTrans.setOrigin(btVector3(btScalar(it->mTransform.x + Offset.x), btScalar(it->mTransform.y + Offset.y), btScalar(it->mTransform.z + Offset.z)));
		else
			btTrans.setOrigin(btVector3(btScalar(Offset.x), btScalar(Offset.y), btScalar(Offset.z)));
	

		btScalar mass(Mass);
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0, 0, 0);
		if (isDynamic)
			mShape->calculateLocalInertia(mass, localInertia);

		//using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
		btDefaultMotionState* myMotionState = new btDefaultMotionState(btTrans);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, mShape, localInertia);
		mBody = new btRigidBody(rbInfo);
}

FCE::Transform::Transform(glm::vec3 transform, glm::vec3 Size,glm::vec2 Size2D, glm::mat4 Rot,float Rotation2D)
{
	mTransform = transform;
	mSize = Size;
	mSize2D = Size2D;
	mRotation = Rot;
	mRotation2D = Rotation2D;
}


