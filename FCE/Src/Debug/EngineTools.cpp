#include "Core/FCEpch.h"
#include "Debug/EngineTools.h"
#include "Core/Engine.h"

#include "Maps/Level.h"
#include "Maps/World.h"

#include "imgui-docking/imgui_internal.h"
#include "imgui-docking/misc/cpp/imgui_stdlib.h"

#include "Components/CameraComponent.h"
#include "Components/CommonComponents.h"
#include "Systems/CollisionSystem.h"

#include "Core/Input.h"



FCE::BasicEngineUtilities::BasicEngineUtilities()
{
    SetName("BasicEngineUtilities");
}

void FCE::BasicEngineUtilities::Display()
{
    if (ImGui::CollapsingHeader("Worlds"))
    {
        if (!FCE::Engine::GetActiveWorld())
            ImGui::Text("No Active World");
        else
        {
            if (ImGui::Button("Reload World"))
                Reload = true;
            if (ImGui::Button("Reload With Current Player Position"))
            {
                Reload = true;
                KeepPlayerPos = true;
            }
            ImGui::ListBox("Load Functions",&currentItem,SelectedLoadFunction.data(),SelectedLoadFunction.size());
            ImGui::InputText(" : Chosen Level", &ChosenLevel);
            ImGui::InputText(" : Chosen World", &ChosenWorld);
            
            if (ImGui::Button("Load Chosen"))
                LoadWorld = true;
        }
    }

    if (ImGui::Checkbox("Debug Camera Enabled", &mIsDebugCamera))
    {
        auto Comp2d = FCE::Engine::GetRegistery()->try_get<Camera2DComponent>(FCE::Engine::GetActiveCameraHandle());
        if (Comp2d)
            FCE::Engine::GetRegistery()->get_or_emplace<Camera2DComponent>(mDebugCamera) = *Comp2d;

        auto Comp3d = FCE::Engine::GetRegistery()->try_get<Camera3DComponent>(FCE::Engine::GetActiveCameraHandle());
        if (Comp3d)
            FCE::Engine::GetRegistery()->get_or_emplace<Camera3DComponent>(mDebugCamera) = *Comp3d;
    }
    ImGui::SliderFloat("DebugCamera Speed multiplier", &mDebugCameraMultiplier, 1.f, 5.f);

    ImGui::Checkbox("Pause Game", &mPaused);

    if (!mPaused)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    //step trough when either button or key is pressed

    if (ImGui::Button("step", ImVec2(100, 50)))
        mSingleFrameCalled = true;

    if (!mPaused)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void FCE::BasicEngineUtilities::RegisterLoadFunction(const char* name, std::function<void(Level*)> loadFunction)
{
    mLevelLoadFunctions.insert(std::pair<const char*, std::function<void(Level*)> >(name, loadFunction));
    SelectedLoadFunction.push_back(name);
}

void FCE::BasicEngineUtilities::DebugCameraInput(float DT)
{
    auto gRegistery = FCE::Engine::GetRegistery();

    auto& Camera2D = gRegistery->get<Camera2DComponent>(mDebugCamera);
    auto& Camera3D = gRegistery->get<Camera3DComponent>(mDebugCamera);

    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_W))
    {
        Camera2D.mPosition += glm::vec4(0, -1, 0, 0) * DT *mDebugCameraMultiplier * mCameraMovement;
        Camera3D.mPosition += Camera3D.mLookAtDir * DT * mDebugCameraMultiplier * mCameraMovement;

    }
    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_S))
    {
        Camera2D.mPosition += glm::vec4(0, 1, 0, 0) * DT * mDebugCameraMultiplier * mCameraMovement;
        Camera3D.mPosition -= Camera3D.mLookAtDir * DT * mDebugCameraMultiplier * mCameraMovement;
    }
    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_A))
    {
        Camera2D.mPosition += glm::vec4(-1, 0, 0, 0) * DT * mDebugCameraMultiplier * mCameraMovement;
        Camera3D.mPosition -= glm::cross(Camera3D.mLookAtDir, Camera3D.mUp) * DT * mDebugCameraMultiplier * mCameraMovement;
    }
    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_D))
    {
        Camera2D.mPosition += glm::vec4(1, 0, 0, 0) * DT * mDebugCameraMultiplier * mCameraMovement;
        Camera3D.mPosition += glm::cross(Camera3D.mLookAtDir, Camera3D.mUp) * DT * mDebugCameraMultiplier * mCameraMovement;
    }

    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_UP))
    {
        glm::vec3 right = glm::cross(Camera3D.mLookAtDir,glm::vec3(0,1,0));
        auto Quat = glm::angleAxis(glm::radians(10 * DT * mDebugCameraMultiplier), right);
        Camera3D.mLookAtDir = glm::rotate(Quat, Camera3D.mLookAtDir);
        Camera3D.mUp = glm::rotate(Quat, Camera3D.mUp);
    }

    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_DOWN))
    {
        glm::vec3 right = glm::cross(Camera3D.mLookAtDir, glm::vec3(0, 1, 0));
        auto Quat = glm::angleAxis(glm::radians(-10 * DT * mDebugCameraMultiplier), right);
        Camera3D.mLookAtDir = glm::rotate(Quat, Camera3D.mLookAtDir);
        Camera3D.mUp = glm::rotate(Quat, Camera3D.mUp);
    }

    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_RIGHT))
    {
        auto Quat = glm::angleAxis(glm::radians(-10 * DT * mDebugCameraMultiplier), Camera3D.mUp);
        Camera3D.mLookAtDir = glm::rotate(Quat, Camera3D.mLookAtDir);
    }

    if (FCE::Engine::GetInputHandler()->IsButton(ButtonStates::HELD, GLFW_KEY_LEFT))
    {
        auto Quat = glm::angleAxis(glm::radians(10 * DT * mDebugCameraMultiplier), Camera3D.mUp);
        Camera3D.mLookAtDir = glm::rotate(Quat, Camera3D.mLookAtDir);
    }

}

void FCE::BasicEngineUtilities::CheckWorldUpdates()
{
    if (Reload)
    {
        auto view = FCE::Engine::GetRegistery()->view<FCE::PlayerComponent>();
        entt::entity player = entt::entity(0);
        if(!view.empty())
        player = view[0];

        auto trans = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(player);
        FCE::Transform T;
        if (trans)
            T = *trans;

        FCE::Engine::GetActiveWorld()->Reload();
        if (KeepPlayerPos)
        {
            view = FCE::Engine::GetRegistery()->view<FCE::PlayerComponent>();
            if (!view.empty())
                player = view[0];

            auto transform = FCE::Engine::GetRegistery()->try_get<FCE::Transform>(player);
            if (transform)
            {
                *transform = T;
                CollisionSystem::UpdatePosition(player);
            }

        }
    }
    if (LoadWorld)
    {
        
        std::string WorldPath = std::string(ChosenWorld);
        std::string LevelPath = std::string(ChosenLevel);
        if (!(LevelPath == ""))
        {
            if (WorldPath == "")
                WorldPath = FCE::Engine::GetActiveWorld()->GetFileName();

            if (std::string(FCE::Engine::GetActiveWorld()->GetFileName()) != WorldPath)
                FCE::Engine::GetActiveWorld()->LoadWorld(WorldPath.c_str());
            if (!SelectedLoadFunction.empty())
            {
                auto func = mLevelLoadFunctions.find(SelectedLoadFunction[currentItem]);
                if (func != mLevelLoadFunctions.end())
                    FCE::Engine::GetActiveWorld()->LoadLevel(LevelPath.c_str(), func->second);
            }
        }
    }

    Reload = false;
    KeepPlayerPos = false;
    LoadWorld = false;
}

FCE::BasicRenderUtilities::BasicRenderUtilities()
{
    SetName("Basic Render Utilities");
}

void FCE::BasicRenderUtilities::Display()
{
    if (ImGui::Checkbox("Debug Drawing", &mRenderDebug))
        FCE::CollisionSystem::SetDebugDraw(mRenderDebug);

    bool renderMode = FCE::Engine::GetRenderMode();
    if (ImGui::Checkbox("Render 3D", &renderMode))
        FCE::Engine::SetRenderMode(renderMode);

    if (ImGui::CollapsingHeader("RenderComponents"))
    {
        if (FCE::Engine::GetRenderMode())
        {
            auto view = FCE::Engine::GetRegistery()->view<FCE::Render3DComponent>();
            for (auto [Handle, RenderComp] : view.each())
            {
                ImGui::Checkbox(std::to_string((int)Handle).c_str(), &RenderComp.mDisabled);
            }
        }
        else
        {
            auto view = FCE::Engine::GetRegistery()->view<FCE::Render2DComponent>();
            for (auto [Handle, RenderComp] : view.each())
            {
                ImGui::Checkbox(std::to_string((int)Handle).c_str(), &RenderComp.mDisabled);
            }
        }
        
    }
  

}
