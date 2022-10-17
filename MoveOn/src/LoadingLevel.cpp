#include "PCH.h"
#include "LoadingLevel.h"

#include "EnemyFunction.h"
#include "InputFunctions.h"

#include "Systems/RenderSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/InputSystem.h"
#include "Systems/CameraSystem.h"

#include "Components/MovementComponent.h"
#include "Components/CommonComponents.h"
#include "Components/LevelTransitionComponent.h"
#include "Components/EnemyComponent.h"
#include "Components/CameraComponent.h"

#include "Maps/World.h"
#include "maps/Level.h"

#include "btBulletDynamicsCommon.h"
#include "btBulletCollisionCommon.h"

#include "LDtkLoader/World.hpp"

#include "Core/Core.h"

#include "WeaponComponent.h"
#include "HealthComponent.h"

#include "Header/VulkanFrontEnd.h"

#include "AStar.h"

entt::entity Player;
entt::entity Camera;

void PlayerDeathFunc(entt::entity e)
{
    FCE::Engine::GetActiveWorld()->GetLevel()->RequestReload = true;
}

void LTtrigger(entt::entity self, entt::entity other)
{
    auto PC = FCE::Engine::GetRegistery()->try_get<FCE::PlayerComponent>(other);
    if (!PC)
        return;

    auto LT = FCE::Engine::GetRegistery()->try_get<FCE::LevelTransition>(self);
    if (LT)
        LT->LoadLevel = true;
}

struct TileSheet
{
    const char* path;
    glm::vec2 Size;
    glm::vec2 BaseOffset;
    glm::vec2 Offset;
    glm::vec2 TileSize;

    glm::vec2 GetOffset(glm::vec2 RowCollum)
    {
        return BaseOffset + (Offset + TileSize) * RowCollum;
    }
};

TileSheet CharacterSheet;
TileSheet WallSheet;

entt::entity CreateGun(std::vector<entt::entity>& entitiesToDelete, entt::registry* ECS,entt::entity owner,float damage, float cooldown)
{
    //Gun
    entt::entity Gun = ECS->create();
    FCE::RenderSystem::Add3DComponent(Gun, "Models/gunV1.obj", "textures/gunV1.png",
        "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv",glm::vec3(0,-32,0));

    FCE::RenderSystem::Add2DComponentSpriteSheet(Gun, 32, 32, CharacterSheet.GetOffset({3,1}),CharacterSheet.Size,CharacterSheet.TileSize,glm::vec2(1),
        CharacterSheet.path,"Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");

    auto trans = ECS->emplace<FCE::Transform>(Gun, glm::vec3(0, 0, 0), glm::vec3(8),glm::vec2(1),glm::rotate(glm::mat4(1.f),glm::radians(180.f),glm::vec3(0,0,1))
        * glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0, 1, 0)), 0.f);
    ECS->emplace<RotateAround2D>(Gun, owner, 64.f, true, 0.f,trans.mRotation);
    ECS->emplace<WeaponComponent>(Gun, damage, cooldown, 250.f, COLLISION_TYPE_BULLET);

    ECS->get_or_emplace<WeaponOwner>(owner, Gun);
    return Gun;
    //Gun
}

void Load(FCE::Level* level)
{
    std::vector<FCE::AStarPoint> Grid;

    int width;
    int height;
    int CellSize = 64;
    level->GetSize(width, height);
    width /= CellSize;
    height /= CellSize;

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            Grid.push_back(FCE::AStarPoint(glm::vec3(x * CellSize, y * CellSize, 0), 1));

    //Load The bullet mesh already so we dont have any loading during gameplay
    FCE::Engine::GetRenderer()->GetMesh("Models/wooden_sphere.obj");
    FCE::Engine::GetRenderer()->GetTexture("Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv","textures/wood_Mat_BaseColor.png", true);

    CharacterSheet.BaseOffset = glm::vec2(1, 0);
    CharacterSheet.Offset = glm::vec2(0, 0);
    CharacterSheet.TileSize = glm::vec2(24, 24);
    CharacterSheet.Size = glm::vec2(216, 72);
    CharacterSheet.path = "textures/kenney_pixelplatformer/Tilemap/characters_packed.png";

    WallSheet.path = "textures/kenney_pixelplatformer/Tilemap/tiles_packed.png";
    WallSheet.BaseOffset = glm::vec2{ 0 };
    WallSheet.Offset = glm::vec2{ 0 };
    WallSheet.Size = glm::vec2{ 360,162 };
    WallSheet.TileSize = glm::vec2{ 18,18 };

    int vikingScaling = CellSize * 0.5f;
    std::vector<entt::entity> entitiesToDelete;
    entt::registry* ECS = FCE::Engine::GetRegistery();
    //loading the player
    {
        std::vector<FCE::EntityData> Ent = level->LoadEntities("EntityLayer", "Player", true);
        if (Ent.size() != 1)
            std::cout << "error with the amount of players in a level" << std::endl;
        for (int i = 0; i < Ent.size(); i++)
        {
            //Player
            Player = Ent[i].Entity;
            const auto& trans = ECS->emplace<FCE::Transform>(Player, Ent[i].Pos, glm::vec3(0.5f),glm::vec2(1,1), glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(0, 1, 0)));
            auto cam = ECS->try_get<FCE::Camera3DComponent>(Camera);
            cam->mPosition = trans.mTransform - cam->mLookAtDir * 10.f;

            FCE::RenderSystem::Add3DComponent(Player, "Models/mario_obj.obj", "textures/marioD.jpg",
                "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv",glm::vec3(16,-32,-32));

            FCE::RenderSystem::Add2DComponentSpriteSheet(Player, Ent[i].Size.x, Ent[i].Size.y, CharacterSheet.GetOffset({1,0}),CharacterSheet.Size,CharacterSheet.TileSize,glm::vec2(1,1),
                CharacterSheet.path,"Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");

            FCE::CollisionSystem::AddComponent(Player, glm::vec3(Ent[i].Size.x * 0.5f, Ent[i].Size.y * 0.5f, Ent[i].Size.x * 0.5f),
                glm::vec3(0.f, 0.f, 0.f), 85, COLLISION_TYPE_PLAYER);
            auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(Player);
            coll->mBody->setFlags(DISABLE_DEACTIVATION);


            ECS->emplace<FCE::PlayerComponent>(Player, Player);
            FCE::Engine::GetRegistery()->emplace<HealthComponent>(Player, 100.f,PlayerDeathFunc);

          
            FCE::Engine::GetRegistery()->emplace<FCE::VelocityComponent>(Player,0.5f,0.0f,glm::vec3(0,0,0),true);
            FCE::InputSystem::AddComponent(Player, Player2DInput);
            //player
            entt::entity Gun = CreateGun(entitiesToDelete, ECS, Ent[i].Entity, 50.f, 0.5f);
            FCE::InputSystem::AddComponent(Gun, Gun2DInput);

        }
    }
    //Loading Enemy
    {
        std::vector<FCE::EntityData> Ent = level->LoadEntities("EntityLayer", "Enemy", true);

        for (int i = 0; i < Ent.size(); i++)
        {
            entt::entity ent = Ent[i].Entity;
            //Transform
            FCE::Engine::GetRegistery()->emplace<FCE::Transform>(ent, Ent[i].Pos, glm::vec3(vikingScaling),glm::vec2(1), glm::rotate(glm::mat4(1),glm::radians(90.f),glm::vec3(0,1,0)));
            //Rendering
            FCE::RenderSystem::Add3DComponent(ent, "Models/slime.obj", "textures/Texture.png",
            "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv");

            FCE::RenderSystem::Add2DComponentSpriteSheet(ent, Ent[i].Size.x, Ent[i].Size.y, CharacterSheet.GetOffset({8,2}),CharacterSheet.Size,CharacterSheet.TileSize,glm::vec2(1,1),
            CharacterSheet.path,"Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");

            //collision
            FCE::CollisionSystem::AddComponent(ent, glm::vec3(32, 32, 32), glm::vec3(0.f, 0.f, 0.f), 85, COLLISION_TYPE_ENEMY);
            auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(ent);
            coll->mBody->setGravity(FCE::glmToBullet(glm::vec3(0, 0, 0)));
       
            //EnemyComponent
            FCE::Engine::GetRegistery()->emplace<FCE::EnemyComponent>(ent, 50.f,DefaultEnemyFunction);
            
            //health
            FCE::Engine::GetRegistery()->emplace<HealthComponent>(ent,100);

            //steering forces
            float val[FCE::SF_LAST + 1];
            val[FCE::SF_SEPARATION] = 20;
            val[FCE::SF_SMOOTHING] = 0.6f;
            val[FCE::SF_WALLAVOIDANCE] = 10;
            FCE::Engine::GetRegistery()->emplace<FCE::SteeringForce>(ent, FCE::DISABLE_ALLIGNMENT | FCE::DISABLE_COHESION, 100.f,val,0.0f,64.f);
           
            //PathFinding
            const auto& PathFinding = FCE::Engine::GetRegistery()->emplace<FCE::AStarComponent>(ent,3.f,16.f,64.f); 
         
            //gun
            CreateGun(entitiesToDelete, ECS, Ent[i].Entity, 25.f, 2.5f);
        }
    }
    //loading the end box
    {
        std::vector < std::reference_wrapper <ldtk::Entity>> entities;
        std::vector<FCE::EntityData> Ent = level->LoadEntities("EntityLayer", "LoadNewLevelBox", true,entities);

        for (int i = 0; i < Ent.size(); i++)
        {
            glm::vec2 Offset = (Ent[i].Size / 64.f);
            glm::vec2 test = Ent[i].Size * ((Offset- glm::vec2(1,1)) / Offset);
            std::string mapPath = entities[i].get().getField<std::string>(std::string("map")).value();
            std::string levelPath = entities[i].get().getField<std::string>(std::string("Level")).value();
            
            std::string TemporaryWorldString = "Worlds/";
            TemporaryWorldString.append(mapPath);
            std::string TemporaryLevelString = levelPath;

            FCE::Engine::GetRegistery()->emplace<FCE::Transform>(Ent[i].Entity, Ent[i].Pos + glm::vec3(test * 0.5f,0), glm::vec3(vikingScaling, vikingScaling, vikingScaling),glm::vec2(1,1), glm::mat4(1));
            FCE::CollisionSystem::AddComponent(Ent[i].Entity, glm::vec3(Ent[i].Size.x * 0.5f, Ent[i].Size.y * 0.5f, 5), glm::vec3(0.f, 0.f, 0.f), 0, COLLISION_TYPE_HITBOX,LTtrigger);
            auto LT = FCE::Engine::GetRegistery()->emplace<FCE::LevelTransition>(
                Ent[i].Entity,level->GetWorld(), TemporaryWorldString, TemporaryLevelString, Load);
            LT.mLoadFunction = Load;
            auto coll = FCE::Engine::GetRegistery()->try_get<FCE::CollisionComponent>(Ent[i].Entity);
        }
    }
    //loading walls
    {
        {
            std::vector<FCE::GridValues> base = level->LoadGridTiles("BaseLayer");
            for (int i = 0; i < base.size(); i++)
            {
                entt::entity e = FCE::Engine::GetRegistery()->create();
                entitiesToDelete.push_back(e);
            
                int wallSizeX = base[i].Size.x / CellSize;
                int wallSizeY = base[i].Size.y / CellSize;
                for (int x = 0; x < wallSizeX; x++)
                    for (int y = 0; y < wallSizeY; y++)
                        Grid[(int)base[i].GridPos.x + x + ((int)base[i].GridPos.y + y) * width].mValue = -1;
                  
                FCE::Engine::GetRegistery()->emplace<FCE::Transform>(e, base[i].Pos, glm::vec3(base[i].Size * 0.5f, vikingScaling),
                    glm::vec2(1), glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(1, 0, 0)));

                FCE::RenderSystem::Add3DComponent(e, "Models/Grass_Block.obj",
                    "textures/Grass_Block_TEX.png", "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv", -1.f * glm::vec3(0.f, base[i].Size.y * 0.5f, 0));
                FCE::RenderSystem::Add2DComponentSpriteSheet(e, CellSize, CellSize, WallSheet.GetOffset({ 6,0 }), WallSheet.Size, WallSheet.TileSize, base[i].Size / glm::vec2(CellSize, CellSize), WallSheet.path,
                    "Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");
                FCE::CollisionSystem::AddComponent(e, glm::vec3(base[i].Size * 0.5f, 32), glm::vec3(0, 0, 0), 0.0f, COLLISION_TYPE_WALL);
            }
            level->AddEntitiesToDeletionQueue(entitiesToDelete);
        }

        {
            std::vector<FCE::GridValues> base = level->LoadFromTileSheet("Walls");
            for (int i = 0; i < base.size(); i++)
            {
                int wallSizeX = base[i].Size.x / CellSize;
                int wallSizeY = base[i].Size.y / CellSize;
                for (int x = 0; x < wallSizeX; x++)
                    for (int y = 0; y < wallSizeY; y++)
                        Grid[(int)base[i].GridPos.x + x + ((int)base[i].GridPos.y + y) * width].mValue = -1;

                entt::entity e = FCE::Engine::GetRegistery()->create();
                entitiesToDelete.push_back(e);

                FCE::Engine::GetRegistery()->emplace<FCE::Transform>(e, base[i].Pos, glm::vec3(base[i].Size * 0.5f, vikingScaling),
                    glm::vec2(1), glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(1, 0, 0)));

                FCE::RenderSystem::Add3DComponent(e, "Models/Grass_Block.obj",
                    "textures/Grass_Block_TEX.png", "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv", -1.f * glm::vec3(0.f, base[i].Size.y * 0.5f, 0));
                FCE::RenderSystem::Add2DComponentSpriteSheet(e, CellSize, CellSize, base[i].TileMap, WallSheet.Size, WallSheet.TileSize, base[i].Size / glm::vec2(CellSize, CellSize), WallSheet.path,
                    "Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");
                FCE::CollisionSystem::AddComponent(e, glm::vec3(base[i].Size * 0.5f, 10), glm::vec3(0, 0, 0), 0.0f, COLLISION_TYPE_WALL);
            }
            level->AddEntitiesToDeletionQueue(entitiesToDelete);
        }

    }

    FCE::CameraSystem::AddFollowComponent(Camera, GetPlayer(), 0.5f, false, glm::vec3(0.f,0.f,200.f));

    FCE::AStarSystem::FillGrid(Grid, width, glm::vec3(CellSize, CellSize, 0));
}

entt::entity GetPlayerCamera()
{
    return Camera;
}

void SetPlayerCamera(entt::entity e)
{
    Camera = e;
}

entt::entity GetPlayer()
{
    return Player;
}