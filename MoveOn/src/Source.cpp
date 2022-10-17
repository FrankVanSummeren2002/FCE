#include "PCH.h"
#include "LoadingLevel.h"

#include "Systems/RenderSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/CameraSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/MovementSystem.h"
#include "Systems/EnemySystem.h"
#include "HealthSystem.h"
#include "WeaponSystem.h"

#include "EnemyFunction.h"

#include "Components/CameraComponent.h"
#include "WeaponComponent.h"

#include "Header/VulkanFrontEnd.h"

#include "Debug/EngineTools.h"

#include "Maps/World.h"
#include "Maps/Level.h"

#include "LoadingLevel.h"

#include "Core/Input.h"

#include "AStar.h"
entt::entity AstarTest;
void DestroyRandomMovement(entt::registry& reg, entt::entity e)
{
    auto D = FCE::Engine::GetRegistery()->get<RandomMovement>(e);
    FCE::TimeHandler::DeleteTimer(D.mInterval);
}

void DestroyWeaponOwner(entt::registry& reg, entt::entity e)
{
    auto weapon = reg.try_get<WeaponOwner>(e);
    
    if (weapon)
        if(reg.valid(weapon->mWeapon))
        reg.destroy(weapon->mWeapon);
}

void GameInit()
{
    FCE::Engine::GetRegistery()->on_destroy<RandomMovement>().connect<DestroyRandomMovement>();
    FCE::Engine::GetRegistery()->on_destroy<WeaponOwner>().connect<DestroyWeaponOwner>();
    FCE::Engine::GetBasicEngineUtilities()->RegisterLoadFunction("Default", Load);

    FCE::RenderSystem::Init();
    FCE::CollisionSystem::Init(glm::vec3(0,980,0));
    FCE::EnemySystem::Init(0.5f, COLLISION_TYPE_WALL);

    FCE::Engine::SetRenderMode(false);
    entt::registry* ECS = FCE::Engine::GetRegistery();
    SetPlayerCamera(ECS->create());
    entt::entity Camera = GetPlayerCamera();
    //add the camera
    glm::mat4 projection = glm::perspective(glm::radians(70.f), FCE::Engine::GetRenderer()->GetScreenSize().x / FCE::Engine::GetRenderer()->GetScreenSize().y, 0.1f, 2000.0f);
    projection[1][1] *= -1;
    FCE::CameraSystem::Add3DComponent(Camera, glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f), projection);
    FCE::CameraSystem::Add2DComponent(Camera, glm::vec4(0, 0, 0, 0));
    //add movement to the camera
    //FCE::InputSystem::AddComponent(Camera,CameraInput);

   
    FCE::CollisionMaskTable CollisionTable;
    CollisionTable.Add(COLLISION_TYPE_WALL, COLLISION_TYPE_PLAYER);
    CollisionTable.Add(COLLISION_TYPE_WALL, COLLISION_TYPE_ENEMY);

    CollisionTable.Add(COLLISION_TYPE_PLAYER, COLLISION_TYPE_HITBOX);
    CollisionTable.Add(COLLISION_TYPE_PLAYER, COLLISION_TYPE_ENEMY);

    CollisionTable.Add(COLLISION_TYPE_BULLET, COLLISION_TYPE_ENEMY);
    CollisionTable.Add(COLLISION_TYPE_BULLET, COLLISION_TYPE_PLAYER);
    CollisionTable.Add(COLLISION_TYPE_BULLET, COLLISION_TYPE_WALL);

    FCE::CollisionSystem::SetCollisionTable(CollisionTable);

    //load the level
    FCE::Engine::SetActiveWorld(new FCE::World("Worlds/FirstLevel.ldtk"));
   FCE::Engine::GetActiveWorld()->LoadLevel("Level_0", Load);   
}

bool GameLoop(float DT) 
{
    FCE::InputSystem::Update(DT);
    FCE::EnemySystem::Update(DT);
    FCE::MovementSystem::UpdateMovement(DT);
    FCE::CollisionSystem::Update(DT);
    HealthSystem::Update();
    WeaponSystem::Update(DT);
    FCE::RenderSystem::Update();
    FCE::CameraSystem::Update(DT);
    FCE::AStarSystem::VisualisePaths(glm::vec3(0, 1, 0));
    FCE::AStarSystem::VisualiseGrid(glm::vec3(0, 1, 1), glm::vec3(1, 1, 0));
	return true;
}
void CleanUp()
{
    FCE::CollisionSystem::CleanUp();
}

int main()
{	
    FCE::Engine::Run(GameInit,GameLoop,CleanUp);
	return 0;
}
