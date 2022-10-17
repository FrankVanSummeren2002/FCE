#include "Core/FCEpch.h"
#include "Core/Engine.h"
#include "Core/Input.h"

#include "Header/VulkanFrontEnd.h"

#include "chrono"

#include "Components/CameraComponent.h"
#include "Components/LevelTransitionComponent.h"

#include "UI/ImGuiHandler.h"
#include "UI/DebugInspectable.h"
#include "Components/InputComponent.h"

#include "Systems/CollisionSystem.h"

#include "Core/Timing.h"

#include "Maps/Level.h"
#include "Maps/World.h"

#include "Debug/EngineTools.h"

using namespace FCE;
FVR::VulkanFrontEnd* gRenderer;
entt::registry* gRegistery;
FCE::Input* gInputHandler;
//make sure that the entt 0 stays consumed
entt::entity Invalid;
entt::entity gCameraHandle;
FCE::World* gWorld = nullptr;
bool gRender3D = true;
std::function<bool(float DT)>gGameLoop;

BasicEngineUtilities* gDebugUtils = nullptr;
BasicRenderUtilities* gRenderUtils = nullptr;

void FCE::Engine::Init()
{
    gRegistery = new entt::registry();
    Invalid = gRegistery->create();
    ImGuiHandler::Init(gRenderer->GetWindow());
    gDebugUtils = new BasicEngineUtilities();
    gRenderUtils = new BasicRenderUtilities();
    gRenderer = new FVR::VulkanFrontEnd();
    gRenderer->Init();

    gInputHandler = new FCE::Input();
    gInputHandler->Init(FCE::Engine::GetRenderer());
  
    gDebugUtils->mDebugCamera = gRegistery->create();
    gRegistery->emplace<Camera3DComponent>(gDebugUtils->mDebugCamera);
    gRegistery->emplace<Camera2DComponent>(gDebugUtils->mDebugCamera);
}

entt::entity FCE::Engine::ChooseCamera()
{
#ifdef NDEBUG
    return gCameraHandle;
#endif // NDEBUG

    if(gDebugUtils->mIsDebugCamera)
    return gDebugUtils->mDebugCamera;

    return gCameraHandle;
}

bool Engine::Update(float DT)
{
    //do the game loop
    bool Open = gGameLoop(DT);
    
    TimeHandler::Update(DT);

    //check for level transitions
    {
        auto LevelTransition = gRegistery->view<FCE::LevelTransition>();
        for (auto [handle, LT] : LevelTransition.each())
        {
            if (LT.LoadLevel)
            {
                DefaultLevelTransition(handle);
                break;
            }
        }

        gDebugUtils->CheckWorldUpdates();
    }

    return Open;
}
void FCE::Engine::Run(std::function<void()>GameInit, std::function<bool(float DT)>GameLoop, std::function<void()>CleanUp)
{
    gGameLoop = GameLoop;

    Init();
    GameInit();

    //TIMING
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto frameTimeBegin = std::chrono::high_resolution_clock::now();
    double frameTime = 0.0;
    double frameTimeSmooth = 0.0;
    
    //the loop
    bool shouldStayOpen = true;
    while (shouldStayOpen)
    {
        //TIMING
        auto newTime = std::chrono::high_resolution_clock::now();
        double elapsed = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(newTime - currentTime).count() / 1'000'000'000.0);
        currentTime = newTime;
        //make sure the elapsed time is not to big after things like breakpoints or screen resizing
        if (elapsed > 1.f / 10.f)
        {
            elapsed = 1.f / 10.f;
        }
        //update input
        gInputHandler->Update((float)elapsed);

        //Check if the window should close
        shouldStayOpen = !FCE::Engine::GetRenderer()->ShouldCloseWindow();

        //call begin frame
        FCE::Engine::GetRenderer()->BeginFrame(!gDebugUtils->mPaused || gDebugUtils->mSingleFrameCalled);

        if (!gDebugUtils->mPaused)
        {
            //call the update
            bool Open = Update((float)elapsed);
            shouldStayOpen = shouldStayOpen & Open;
        }
        else if (gDebugUtils->mSingleFrameCalled)
        {
            bool Open = Update(1.f / 60.f);
            shouldStayOpen = shouldStayOpen & Open;
            gDebugUtils->mSingleFrameCalled = false;
        }

        if (gWorld)
        {
            if(gWorld->GetLevel())
            if (gWorld->GetLevel()->RequestReload)
                gWorld->GetLevel()->Reload();
        }
        //input stuff for when debugging
        if (gDebugUtils->mIsDebugCamera)
            gDebugUtils->DebugCameraInput((float)elapsed);

#ifndef NDEBUG
        if (FCE::Engine::GetInputHandler()->IsButton(FCE::PRESSED, GLFW_KEY_P))
            gDebugUtils->mPaused = !gDebugUtils->mPaused;
        if (FCE::Engine::GetInputHandler()->IsButton(FCE::PRESSED, GLFW_KEY_LEFT_BRACKET) && gDebugUtils->mPaused)
            gDebugUtils->mSingleFrameCalled = true;
#endif // !NDEBUG

        //update all of the imgui stuff
            ImGuiHandler::Update();
        if (FCE::Engine::GetRenderMode())
        {
            Camera3DComponent* cam = gRegistery->try_get<Camera3DComponent>(Engine::ChooseCamera());
            if (cam)
                FCE::Engine::GetRenderer()->EndFrame(glm::lookAt(cam->mPosition, cam->mPosition + cam->mLookAtDir, cam->mUp), cam->mProjMatrix);
            else
            {
                //A projection matrix used for when no camera is selected
                glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
                projection[1][1] *= -1;
                std::cout << "No Active Camera" << std::endl;
                FCE::Engine::GetRenderer()->EndFrame(glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 1, 0)), projection);
            }
        }
        else
        {
            auto cam = gRegistery->try_get<Camera2DComponent>(Engine::ChooseCamera());
            if (cam)
            {
                FCE::Engine::GetRenderer()->EndFrame2D(cam->mPosition);
            }
            else
            {
                std::cout << "No Active Camera" << std::endl;
                FCE::Engine::GetRenderer()->EndFrame2D(glm::vec4(0, 0, 0, 0));
            }
        }

        //TIMING
        auto frameTimeEnd = std::chrono::high_resolution_clock::now();
        frameTime = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(frameTimeEnd - frameTimeBegin).count() / 1'000'000'000.0);
        frameTimeBegin = frameTimeEnd;
        frameTimeSmooth = 0.1f * frameTime + 0.9f * frameTimeSmooth;
        //
    }

    FCE::Engine::GetRenderer()->CleanUp();
    CleanUp();
}

bool FCE::Engine::IsDebugging()
{
#ifdef NDEBUG
    return false;
#else // DEBUG
    return true;
#endif
}

void FCE::Engine::SetActiveCameraHandle(entt::entity CameraComponent)
{
    gCameraHandle = CameraComponent;
}
void Engine::SetRenderMode(bool render3D)
{
    gRender3D = render3D;
}
void FCE::Engine::SetActiveWorld(World* world)
{
    gWorld = world;
}

entt::entity FCE::Engine::GetActiveCameraHandle()
{
    return gCameraHandle;
}
Input* FCE::Engine::GetInputHandler()
{
    return gInputHandler;
}
FVR::VulkanFrontEnd* Engine::GetRenderer()
{
    return gRenderer;
}
FCE::World* FCE::Engine::GetActiveWorld()
{
    return gWorld;
}

entt::registry* Engine::GetRegistery()
{
    return gRegistery;
}
bool Engine::GetRenderMode()
{
    return gRender3D;
}
bool Engine::GetDebugDrawing() { return gRenderUtils->mRenderDebug; }

BasicEngineUtilities* Engine::GetBasicEngineUtilities()
{
    return gDebugUtils;
}
