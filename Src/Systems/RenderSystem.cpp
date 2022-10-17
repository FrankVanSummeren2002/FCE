#include "Core/FCEpch.h"
#include "Systems/RenderSystem.h"
#include "Core/Engine.h"
#include "Components/CommonComponents.h"
#include "Header/VulkanFrontEnd.h"

using namespace FCE;

FVR::VulkanFrontEnd* mRenderer{ nullptr };
entt::registry* gRegister{ nullptr };

void RenderSystem::Init()
{
    mRenderer = Engine::GetRenderer();
    gRegister = Engine::GetRegistery();
}
void RenderSystem::Add3DComponent(entt::entity Entity,const char* meshPath,const char* texturePath,const char* vertShaderPath,const char* fragShaderPath,glm::vec3 Offset)
{
    FVR::MeshHandle mesh = mRenderer->GetMesh(meshPath);
    FVR::TextureHandle text = mRenderer->GetTexture(vertShaderPath,fragShaderPath,texturePath,true);
    Add3DComponent(Entity, mesh, text,Offset);
}
void RenderSystem::Add2DComponentSingleTexture(entt::entity Entity, uint32_t width, uint32_t height,glm::vec2 repetition, const char* texturePath, const char* vertShaderPath, const char* fragShaderPath)
{
    FVR::QuadHandle quad = mRenderer->GetQuadSingleTexture(width, height);
    FVR::TextureHandle text = mRenderer->GetTexture(vertShaderPath, fragShaderPath, texturePath, false);
    Add2DComponent(Entity, quad, text, glm::vec2(width, height),repetition);
}

void FCE::RenderSystem::Add2DComponentSpriteSheet(entt::entity Entity, uint32_t width, uint32_t height, glm::vec2 BaseOffset, glm::vec2 sheetSize, glm::vec2 tileSize, glm::vec2 repetition, const char* texturePath, const char* vertShaderPath, const char* fragShaderPath)
{
    FVR::QuadHandle quad = mRenderer->GetQuadSpriteSheet(width, height,BaseOffset,sheetSize,tileSize);
    FVR::TextureHandle text = mRenderer->GetTexture(vertShaderPath, fragShaderPath, texturePath, false);
    Add2DComponent(Entity, quad, text,glm::vec2(width,height), repetition);
}

void RenderSystem::Add3DComponent(entt::entity Entity, FVR::MeshHandle meshHandle, FVR::TextureHandle textHandle,glm::vec3 Offset)
{
    gRegister->emplace<Render3DComponent>(Entity, meshHandle, textHandle,Offset);
}

void FCE::RenderSystem::Add2DComponent(entt::entity entity, FVR::QuadHandle quadHandle, FVR::TextureHandle textHandle,glm::vec2 Size,glm::vec2 repetition)
{
    gRegister->emplace<Render2DComponent>(entity, quadHandle, textHandle,Size, repetition);
}

void RenderSystem::Update()
{
    {
        auto view = gRegister->view<Render3DComponent>();

        for (auto entity : view)
        {
            if (gRegister->all_of<Transform>(entity))
            {
                auto RenderComp = view.get<Render3DComponent>(entity);
                auto transform = gRegister->try_get<Transform>(entity);
                glm::mat4 Model = glm::translate(glm::mat4(1), transform->mTransform - RenderComp.mOffset) * transform->mRotation * glm::scale(glm::mat4(1), transform->mSize);
                if(!RenderComp.mDisabled)
                mRenderer->DrawModel(Model, RenderComp.mMeshHandle, RenderComp.mTextureHandle);
            }
            else
            {
                std::cout << "No Transform component found for the RenderComponent" << std::endl;
            }
        }
    }

    {
        auto view = gRegister->view<Render2DComponent>();

        for (auto entity : view)
        {
            if (gRegister->all_of<Transform>(entity))
            {
                auto RenderComp = view.get<Render2DComponent>(entity);
                auto transform = gRegister->try_get<Transform>(entity);
                if (!RenderComp.mDisabled)
                {
                    for (int x = 0; x < RenderComp.mRepetition.x; x++)
                        for(int y = 0; y < RenderComp.mRepetition.y; y++)
                        {
                            glm::vec2 pos = transform->mTransform;
                            pos -= (RenderComp.mRepetition * 0.5f - glm::vec2(0.5f)) * RenderComp.mSize;
                            pos += RenderComp.mSize * glm::vec2(x,y);
                            mRenderer->DrawQuad(glm::vec4(pos, 0, 0), glm::vec3(transform->mSize2D, 1), RenderComp.mQuadHandle, RenderComp.mTextureHandle, transform->mRotation2D);
                        }
                }
            }
            else
            {
                std::cout << "No Transform component found for the RenderComponent" << std::endl;
            }
        }
    }
}