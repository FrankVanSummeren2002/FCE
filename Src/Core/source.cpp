
#include "Core/FCEpch.h"
//#include <chrono>
//#include "GLFW/glfw3.h"
//#include "imgui-docking/imgui.h"
//#include "Core/Core.h"
//#include "chrono"
//#include "Header/VulkanFrontEnd.h"
//#include "Core/Engine.h"
//#include "Systems/RenderSystem.h"
//#include "Systems/CollisionSystem.h"
//#include "Components/CommonComponents.h"
//#include "Maps/Level.h"
//#include "LDtkLoader/World.hpp"
//#include "Core/Input.h"
//
//
//int main(int argc, char* argv[])
//{
//   // FCE::Engine::Init();
//    FCE::RenderSystem::Init();
//    FCE::CollisionSystem::Init();
//    FCE::Input inputHandler;
//
//    inputHandler.Init(FCE::Engine::GetRenderer());
//    
//    entt::registry* ECS = FCE::Engine::GetRegistery();
//
//    entt::entity Viking = ECS->create();
//    entt::entity MC = ECS->create();
//    entt::entity CHAR = ECS->create();
//
//   FCE::RenderSystem::Add3DComponent(Viking, "Models/viking_room.obj", "textures/viking_room.png", "Shaders/3D/tri_mesh_ssbo.vert.spv", "Shaders/3D/textured_lit.frag.spv");
//   FCE::RenderSystem::Add2DComponent(CHAR,16,16, 16,16, "textures/kenney_pixelplatformer/Tiles/tile_0006.png", "Shaders/2D/tri_mesh_ssbo2D.vert.spv", "Shaders/2D/textured_lit2D.frag.spv");
//   FCE::Engine::SetRenderMode(false);
//   // FCE::RenderSystem::AddComponent(MC, "assets/lost_empire.obj", "textures/lost_empire-RGBA.png", "Shaders/tri_mesh_ssbo.vert.spv", "Shaders/textured_lit.frag.spv");
//   glm::vec3 Pos = glm::vec3(0.0f, -6.0f, -8.0f);
//   ECS->emplace<FCE::Transform>(Viking);
//   ECS->emplace<FCE::Transform>(MC);
//   ECS->emplace<FCE::Transform>(CHAR);
//   //Draw viking
//   {
//       auto& trans = ECS->get<FCE::Transform>(Viking);
//       trans.mRotation = glm::rotate(glm::mat4(1), glm::radians(-90.f), glm::vec3(1, 0, 0));
//       trans.mTransform = Pos;
//       trans.mSize = glm::vec3(1, 1, 1);
//   }
//   //draw MC
//   {
//       auto& trans = ECS->get<FCE::Transform>(MC);
//       trans.mTransform = glm::vec3(-10, 0 - 20, 0);
//       trans.mRotation = glm::rotate(glm::mat4(1), glm::radians(180.f), glm::vec3(0, 1, 0));
//       trans.mSize = glm::vec3(1, 1, 1);
//   }
//
//   //draw CHAR
//   {
//       auto& trans = ECS->get<FCE::Transform>(CHAR);
//       trans.mTransform = glm::vec3(100, 0, 0);
//       trans.mSize = glm::vec3(1, 1, 1);
//   }
//
//   FCE::CollisionSystem::AddComponent(Viking, glm::vec3(0.5, 0.5, 0.5), glm::vec3(0,0,0),0);
//
//    //camera projection
//    float currPos = 0;
//    float Pos2 = 0;
//
//    //TIMING
//    auto currentTime = std::chrono::high_resolution_clock::now();
//    auto frameTimeBegin = std::chrono::high_resolution_clock::now();
//    double frameTime = 0.0;
//    double frameTimeSmooth = 0.0;
//    //
//    
//    bool shouldStayOpen = true;
//    while (shouldStayOpen)
//    {
//        glm::mat4 view = glm::lookAt(glm::vec3(0.f, -6.f + currPos, 10.f + Pos2), glm::vec3(0.0f, -6.0f + currPos, 9.0f + Pos2), glm::vec3(0.0f, 1.0f, 0.0f));
//        glm::mat4 projection = glm::perspective(glm::radians(70.f), FCE::Engine::GetRenderer()->GetScreenSize().x / FCE::Engine::GetRenderer()->GetScreenSize().y, 0.1f, 200.0f);
//        projection[1][1] *= -1;
//
//        //TIMING
//        auto newTime = std::chrono::high_resolution_clock::now();
//        double elapsed = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(newTime - currentTime).count() / 1'000'000'000.0);
//        currentTime = newTime;
//        //
//
//        shouldStayOpen = FCE::Engine::GetRenderer()->BeginFrame() != 1;
//        inputHandler.Update((float) elapsed);
//
//        currPos += 10 * elapsed * inputHandler.IsButton(FCE::PRESSED, GLFW_KEY_W);
//        currPos += -10 * elapsed * inputHandler.IsButton(FCE::PRESSED, GLFW_KEY_S);
//
//        Pos2 += 10 * elapsed * inputHandler.IsButton(FCE::PRESSED, GLFW_KEY_A);
//        Pos2 += -10 * elapsed * inputHandler.IsButton(FCE::PRESSED, GLFW_KEY_D);
//
//
//        FCE::RenderSystem::Update();
//        FCE::CollisionSystem::Update(frameTime);
//        
//        FCE::Engine::GetRenderer()->DrawLine(glm::vec3(0.5f, 0.5f, 0.f), glm::vec3(-0.5f, 0.5f, 0.f), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1));
//
//        if(FCE::Engine::GetRenderMode())
//            FCE::Engine::GetRenderer()->EndFrame(view,projection);
//        else
//            FCE::Engine::GetRenderer()->EndFrame2D(glm::vec4(Pos2,currPos,0,0));
//        //TIMING
//        auto frameTimeEnd = std::chrono::high_resolution_clock::now();
//        frameTime = (double)(std::chrono::duration_cast<std::chrono::nanoseconds>(frameTimeEnd - frameTimeBegin).count() / 1'000'000'000.0);
//        frameTimeBegin = frameTimeEnd;
//        frameTimeSmooth = 0.1f * frameTime + 0.9f * frameTimeSmooth;
//        //
//    }
//   
//    FCE::Engine::GetRenderer()->CleanUp();
//
//    return 0;
//}
//
