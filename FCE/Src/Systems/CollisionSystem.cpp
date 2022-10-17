#include "Core/FCEpch.h"
#include "Systems/CollisionSystem.h"
#include "btBulletDynamicsCommon.h"
#include "Core/Engine.h"
#include "Components/CommonComponents.h"
#include "Debug/BulletDebugDraw.h"
#include "Core/Core.h"
#include "btBulletCollisionCommon.h"

btDiscreteDynamicsWorld* mDynamicsWorld;
btSequentialImpulseConstraintSolver* mSolver;
btBroadphaseInterface* mOverlappingPairCache;
btCollisionDispatcher* mDispatcher;
btDefaultCollisionConfiguration* mCollisionConfiguration;
FCE::BulletDebugDraw* debugDraw;
FCE::CollisionMaskTable gTable;



struct RayResult : public btCollisionWorld::RayResultCallback
{
	btScalar addSingleResult(btCollisionWorld::LocalRayResult& ray_result, bool is_normal_in_world_space)
	{
		(void)is_normal_in_world_space;
		auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>((entt::entity)ray_result.m_collisionObject->getUserIndex());
		if (coll)
		{
			if (coll->collisionGroup & mChannels)
			{
				FCE::RayCollisionData colldata;
				m_collisionObject = ray_result.m_collisionObject;
				colldata.mHitFraction = ray_result.m_hitFraction;
				colldata.mHitNormal = FCE::bulletToGlm(ray_result.m_hitNormalLocal);
				colldata.mEntity = (entt::entity)ray_result.m_collisionObject->getUserIndex();
				mRayCollData.emplace_back(colldata);
			}
		}

		return 0.f;
	}

	uint32_t mChannels;
	std::vector<FCE::RayCollisionData> mRayCollData;
};

struct FirstRayResult : public btCollisionWorld::RayResultCallback
{
	btScalar addSingleResult(btCollisionWorld::LocalRayResult& ray_result, bool is_normal_in_world_space)
	{
		auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>((entt::entity)ray_result.m_collisionObject->getUserIndex());
		if (coll)
		{
			if (coll->collisionGroup & mChannels)
			{
				(void)is_normal_in_world_space;
				mRayCollData.mHitFraction = ray_result.m_hitFraction;
				mRayCollData.mHitNormal = FCE::bulletToGlm(ray_result.m_hitNormalLocal);
				mRayCollData.mEntity = (entt::entity)ray_result.m_collisionObject->getUserIndex();
				m_collisionObject = ray_result.m_collisionObject;
				return ray_result.m_hitFraction;
			}
		}
		return 0.f;
	}

	uint32_t mChannels{ 0 };
	FCE::RayCollisionData mRayCollData{};
};


class MotionState:
	public btMotionState
{
private:
	entt::entity mID;
public:
	MotionState(entt::entity id) : mID{ id } {};

	void getWorldTransform(btTransform& worldTrans) const
	{
		auto trans = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(mID);
		auto col = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(mID);
		if (trans && col)
		{
			glm::vec3 pos = trans->mTransform + col->offset;
			worldTrans.setOrigin(FCE::glmToBullet(pos));
			worldTrans.setRotation(FCE::glmToBullet(trans->mRotation));
		}
	}

	//Bullet only calls the update of worldtransform for active objects
	void setWorldTransform(const btTransform& worldTrans) 
	{
		auto trans = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(mID);
		auto col = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(mID);
		if (trans && col)
		{
			btQuaternion q;
			worldTrans.getBasis().getRotation(q);
			trans->mTransform = FCE::bulletToGlm(worldTrans.getOrigin()) - col->offset;
			trans->mRotation = FCE::bulletToGlm(q);
		}
	}

	const entt::entity GetId() { return mID; }
};

void PushOut(btManifoldPoint point, entt::entity entToPush, entt::entity otherEnt,float inversion)
{
	btRigidBody* toPush = FCE::Engine::GetRegistery()->get<FCE::CollisionComponent>(entToPush).mBody;
	btRigidBody* other = FCE::Engine::GetRegistery()->get<FCE::CollisionComponent>(otherEnt).mBody;
	if (!toPush->isStaticObject())
	{
		//dont do anything with tiny overlaps to make sure you do not keep jithering with things like the ground
		if ((float)point.getDistance() < 0.1f)
		{
			return;
		}

		auto trans = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(entToPush);
		if (trans)
		{
			glm::vec3 Dir = FCE::bulletToGlm(point.m_normalWorldOnB);

			trans->mTransform += Dir * (float)point.m_distance1;
			trans->mTransform.z = 0;
			std::cout << "push out y :" << Dir.y * (float)point.m_distance1 << std::endl;
			FCE::CollisionSystem::UpdatePosition(entToPush);
		}
	
	}
}
void TickCallback(btDynamicsWorld* dynamicsWorld, btScalar timeStep) 
{
	btCollisionObjectArray arr = dynamicsWorld->getCollisionObjectArray();

	int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
	for (int i = 0; i < numManifolds; i++) 
	{

		btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
		{
			const btCollisionObject* objA = contactManifold->getBody0();
			const btCollisionObject* objB = contactManifold->getBody1();


			//todo better way of storing the id
			if (objA && objB)
			{
				entt::entity A = (entt::entity)objA->getUserIndex();
				entt::entity B = (entt::entity)objB->getUserIndex();
				
				auto a = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(A);
				auto b = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(B);
				if(a)
					a->mCollisionCallback(A,B);
				if (b)
					b->mCollisionCallback(B,A);
				
				contactManifold->refreshContactPoints(objA->getWorldTransform(),objB->getWorldTransform());

				

				if (contactManifold->getNumContacts() != 0)
				{
					PushOut(contactManifold->getContactPoint(0), A, B, 1);
					PushOut(contactManifold->getContactPoint(0), B, A, -1);
				}
				
			}
		}
	}
}




void FCE::CollisionSystem::SetCollisionTable(FCE::CollisionMaskTable table)
{
	gTable = table;
}

FCE::CollisionMaskTable FCE::CollisionSystem::GetCollisionTable()
{
	return gTable;
}

void FCE::CollisionSystem::Init(glm::vec3 Gravity)
{
	///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
	mCollisionConfiguration = new btDefaultCollisionConfiguration();

	///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);

	///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
	mOverlappingPairCache = new btDbvtBroadphase();

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	mSolver = new btSequentialImpulseConstraintSolver;

	mDynamicsWorld = new btDiscreteDynamicsWorld(mDispatcher, mOverlappingPairCache, mSolver, mCollisionConfiguration);
	//TODO why is this positive ?
	mDynamicsWorld->setGravity(btVector3(glmToBullet(Gravity)));
	debugDraw = new FCE::BulletDebugDraw();
	mDynamicsWorld->setDebugDrawer(debugDraw);
	mDynamicsWorld->setInternalTickCallback(TickCallback);

	FCE::Engine::GetRegistery()->on_destroy<CollisionComponent>().connect<CollisionSystem::DeleteComponent>();
}

void FCE::CollisionSystem::Update(float DT)
{
	mDynamicsWorld->stepSimulation(DT, 10);

#ifdef NDEBUG

#else
	mDynamicsWorld->debugDrawWorld();
#endif // NDEBUG

}

void FCE::CollisionSystem::UpdatePosition(entt::entity ent)
{
	auto Collision = FCE::Engine::GetRegistery()->try_get<CollisionComponent>(ent);
	auto Transform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(ent);
	if(Collision && Transform)
	{
		btTransform trans;
		trans.setOrigin(glmToBullet(Transform->mTransform));
		trans.setBasis(Collision->mBody->getWorldTransform().getBasis());
		
		Collision->mBody->setWorldTransform(trans);
		Collision->mBody->getMotionState()->setWorldTransform(trans);
		Collision->mBody->activate();
	}
}

void DefaultFunc(entt::entity self, entt::entity other) {};
void FCE::CollisionSystem::AddComponent(entt::entity Entity,glm::vec3 Size, glm::vec3 Offset, float Mass,int group,std::function<void (entt::entity self, entt::entity other)> Callback)
{
	if (!Callback)
		Callback = DefaultFunc;

	auto& Comp = Engine::GetRegistery()->emplace<FCE::CollisionComponent>(Entity,Entity, Size, Offset, Mass,group, gTable.Find(group),Callback);
	Comp.mBody->setMotionState(new MotionState(Entity));
	Comp.mBody->setUserIndex((int)Entity);
	mDynamicsWorld->addRigidBody(Comp.mBody,Comp.collisionGroup,Comp.mCollisionChannels);
	Comp.mBody->setDamping(btScalar(0.5f),btScalar(0.9f));
	Comp.mBody->setAngularFactor(btScalar(0.f));
	Comp.mBody->setLinearFactor(btVector3(btScalar(1.f), btScalar(1.f), btScalar(0.f)));
	
}

void FCE::CollisionSystem::CleanUp()
{
	for (int i = mDynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
	{
		btCollisionObject* obj = mDynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		mDynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	auto view = Engine::GetRegistery()->view<CollisionComponent>();

	for (auto collisionComponent : view)
	{
		auto comp = view.get<CollisionComponent>(collisionComponent);
		delete comp.mShape;
	}

	//delete dynamics world
	delete mDynamicsWorld;
	//delete solver
	delete mSolver;
	//delete broadphase
	delete mOverlappingPairCache;
	//delete dispatcher
	delete mDispatcher;
	delete mCollisionConfiguration;
}

#include "Header/VulkanFrontEnd.h"
bool FCE::CollisionSystem::OnGround(entt::entity ent,glm::vec3 offset)
{
	auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(ent);
	btTransform trans;
	coll->mBody->getMotionState()->getWorldTransform(trans);
	btVector3 start = trans.getOrigin();
	btVector3 end;
	btScalar dist;
	coll->mBody->getCollisionShape()->getBoundingSphere(end,dist);
	end = start;
	end.setY(end.y() + dist + btScalar(1));

	glm::vec3 gStart = bulletToGlm(start);
	glm::vec3 gEnd = bulletToGlm(end);

	return OnGround(coll,gStart + offset,gEnd + offset) || OnGround(coll,gStart - offset, gEnd - offset);
}

bool FCE::CollisionSystem::OnGround(FCE::CollisionComponent* comp,glm::vec3 start,glm::vec3 end)
{
	if (FCE::Engine::GetDebugDrawing())
		FCE::Engine::GetRenderer()->DrawLine(start, end, glm::vec3(1, 0, 1), glm::vec3(1, 0, 0), true);

	FirstRayResult Result;
	Result.mChannels = comp->mCollisionChannels;
	mDynamicsWorld->rayTest(glmToBullet(start), glmToBullet(end), Result);
	return Result.hasHit();
}

std::vector<FCE::RayCollisionData> FCE::CollisionSystem::ShootRay(glm::vec3 Start, glm::vec3 End, uint32_t flags)
{
	if(debugDraw->getDebugMode() > 0)
	FCE::Engine::GetRenderer()->DrawLine(Start, End, glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), true);

	btVector3 start = glmToBullet(Start);
	btVector3 end = glmToBullet(End);
	RayResult Result;
	Result.mChannels = flags;

	if (start == end)
		return Result.mRayCollData;

	mDynamicsWorld->rayTest(start, end, Result);
	return Result.mRayCollData;
}

void FCE::CollisionSystem::DeleteComponent(entt::registry& reg, entt::entity ent)
{
	auto Comp = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(ent);
	if (!Comp)
		return;

	if (Comp->mBody && Comp->mBody->getMotionState())
	{
		delete Comp->mBody->getMotionState();
	}
	mDynamicsWorld->removeCollisionObject(Comp->mBody);
}

void FCE::CollisionSystem::SetDebugDraw(bool debug)
{
	if (debug)
		debugDraw->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	else
		debugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);
	
}
