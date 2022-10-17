#include "PCH.h"
#include "InputFunctions.h"
#include "Core/Input.h"
#include "UI/ImGuiHandler.h"

#include "Components/CommonComponents.h"
#include "Components/CameraComponent.h"
#include "Components/MovementComponent.h"
#include "WeaponComponent.h"

#include "Systems/CameraSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/MovementSystem.h"
#include "WeaponSystem.h"
void CameraInput(entt::entity ent, float DT)
{
    auto Cam2d = FCE::Engine::GetRegistery()->try_get<FCE::Camera2DComponent>(ent);
    if (Cam2d)
    {
        Cam2d->mPosition.x += DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_D);
        Cam2d->mPosition.x -= DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_A);
        Cam2d->mPosition.y -= DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_W);
        Cam2d->mPosition.y += DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_S);
    }
    auto Cam3d = FCE::Engine::GetRegistery()->try_get<FCE::Camera3DComponent>(ent);
    if (Cam3d)
    {
        Cam3d->mPosition.x += DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_D);
        Cam3d->mPosition.x -= DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_A);
        Cam3d->mPosition.y += DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_UP);
        Cam3d->mPosition.y -= DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_DOWN);
        Cam3d->mPosition.z += DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_W);
        Cam3d->mPosition.z -= DT * FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_S);
    }
}

void UpdateDir(glm::vec3& velDir, glm::vec3 newDir,float alpha = 0.5f)
{
    velDir = velDir + (newDir - velDir) * 0.5f;
    if (glm::length(velDir) > 0.1f)
        velDir = glm::normalize(velDir);
}

void Player2DInput(entt::entity ent, float DT)
{
    //movement
    auto vel = FCE::Engine::GetRegistery()->try_get<FCE::VelocityComponent>(ent);
    
    if (FCE::CollisionSystem::OnGround(ent,glm::vec3(32,0,0)))
    {
        if (FCE::Engine::GetInputHandler()->IsButton(FCE::PRESSED, GLFW_KEY_SPACE))
        {
            FCE::MovementSystem::AddImpulse(ent, glm::vec2(0, -50000));
        }
    }
    if (vel)
    {
        vel->mSpeed = 0;
        if (FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_A))
        {
            UpdateDir(vel->mDir, glm::vec3(-1, 0,0));
            vel->mSpeed = 2500000;
        }
       
        if (FCE::Engine::GetInputHandler()->IsButton(FCE::HELD, GLFW_KEY_D))
        {
            UpdateDir(vel->mDir, glm::vec3(1, 0,0));
            vel->mSpeed = 2500000;
        }
    }
}

void Gun2DInput(entt::entity self,float DT)
{
    auto gun = FCE::Engine::GetRegistery()->try_get<WeaponComponent>(self);
    auto rot = FCE::Engine::GetRegistery()->try_get<RotateAround2D>(self);

    //shoot
    if (FCE::Engine::GetInputHandler()->IsMouseButton(FCE::PRESSED, GLFW_MOUSE_BUTTON_1) && !FCE::ImGuiHandler::IsAWindowHovered())
        WeaponSystem::Shoot(self);
 
    //rotate
    const auto& transform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(rot->mTarget);
    if (transform)
    {
        glm::vec2 Pos = transform->mTransform;
        Pos = FCE::CameraSystem::Convert2DPosToScreenSpace(FCE::Engine::GetActiveCameraHandle(), Pos);
        glm::vec2 screenPos = FCE::Engine::GetInputHandler()->GetMouseScreenPos();
        rot->Rotate(Pos, screenPos, self);
    }
}