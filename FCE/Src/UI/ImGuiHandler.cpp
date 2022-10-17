#include "Core/FCEpch.h"

#include "UI/ImGuiHandler.h"
#include "UI/Inspectable.h"
#include "UI/DebugInspectable.h"
#include "Core/Engine.h"
#include "Header/VulkanFrontEnd.h"


//all of the windows
static std::vector<FCE::Inspectable*> gInspectables;
static std::vector<FCE::DebugInspectable*> gDebugInspectables;
static bool gMainWindow = true;
static bool gHovered = false;

// Forward Declarations
static glm::vec2 gMin = glm::vec2(0, 0);
static glm::vec2 gMax = glm::vec2(0, 0);


void FCE::ImGuiHandler::Init(GLFWwindow* aWindow)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking

    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::EmbraceTheDarkness(&style);

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGuiIO& o = ImGui::GetIO();
    o.ConfigFlags;
    // Setup Platform/Renderer backends
}



bool FCE::ImGuiHandler::IsAWindowHovered()
{
    return ImGui::IsAnyItemHovered() || gHovered;
}

void FCE::ImGuiHandler::AddInspectable(Inspectable* inspectable)
{
    for (int i = 0; i < gInspectables.size(); i++)
    {
        if (gInspectables[i]->GetName() == inspectable->GetName())
            return;
    }
    gInspectables.push_back(inspectable);
}

void FCE::ImGuiHandler::AddInspectable(DebugInspectable* inspectable)
{
    for (int i = 0; i < gDebugInspectables.size(); i++)
    {
        if (gDebugInspectables[i]->GetName() == inspectable->GetName())
            return;
    }
    gDebugInspectables.push_back(inspectable);
}

void FCE::ImGuiHandler::RemoveInspectable(Inspectable* inspectable)
{
    for (int i = 0; i < gInspectables.size(); i++)
    {
        if (gInspectables[i] == inspectable)
        {
            gInspectables.erase(gInspectables.begin() + i);
        }
    }
}

void FCE::ImGuiHandler::RemoveInspectable(DebugInspectable* inspectable)
{
    for (int i = 0; i < gInspectables.size(); i++)
    {
        if (gDebugInspectables[i] == inspectable)
        {
            gDebugInspectables.erase(gDebugInspectables.begin() + i);
        }
    }
}

void FCE::ImGuiHandler::Update()
{
  
    //Create the main window
    gHovered = false;
    if (Engine::IsDebugging())
    {
        ImGui::DockSpaceOverViewport();
        MainWindow();

        for (int i = 0; i < gDebugInspectables.size(); i++)
        {
            if (gDebugInspectables[i]->mOpen)
            {
                if (gDebugInspectables[i]->mOverideBeginEnd)
                {
                    gDebugInspectables[i]->Display();
                    if (ImGui::IsWindowHovered())
                        gHovered = true;
                }
                else
                {
                    ImGui::Begin(gDebugInspectables[i]->GetName());
                    gDebugInspectables[i]->Display();
                    if (ImGui::IsWindowHovered())
                        gHovered = true;
                    ImGui::End();
                }
            }
        }
    }

    for (int i = 0; i < gInspectables.size(); i++)
    {
        if (gInspectables[i]->mOpen)
        {
            if (gInspectables[i]->mOverideBeginEnd)
            {
                gInspectables[i]->Display();
                if (ImGui::IsWindowHovered())
                    gHovered = true;
            }
            else
            {
                ImGui::Begin(gInspectables[i]->GetName());
                gInspectables[i]->Display();
                if (ImGui::IsWindowHovered())
                    gHovered = true;
                ImGui::End();
            }
        }
    }
}


void FCE::ImGuiHandler::MainWindow()
{
    glm::vec2 screensize = FCE::Engine::GetRenderer()->GetScreenSize();
    ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(screensize.x, screensize.y), ImGuiCond_FirstUseEver);

    ImGui::Begin("Main Window",NULL,ImGuiWindowFlags_MenuBar);

    ImVec2 Min = ImGui::GetWindowPos();
    ImVec2 Size = ImGui::GetWindowSize();
    //add a bit on the y to negate the Menu bar and the title
    gMin = glm::vec2(Min.x, Min.y + 35);
    gMax = glm::vec2(Min.x, Min.y) + glm::vec2(Size.x, Size.y);
    // Record dear imgui primitives into command buffer
    FCE::Engine::GetRenderer()->DrawImguiViewPort();
 
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Options"))
        {
            for (int i = 0; i < gDebugInspectables.size(); i++)
            {
                if (!gDebugInspectables[i]->mHidden)
                {
                    ImGui::MenuItem(gDebugInspectables[i]->GetName(),NULL, &gDebugInspectables[i]->mOpen);
                    if (ImGui::IsWindowHovered())
                        gHovered = true;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}

bool FCE::ImGuiHandler::GetMainWindowMinMax(glm::vec2& aMin, glm::vec2& aMax)
{
    aMin = gMin;
    aMax = gMax;

    return true;
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
// In your own code you may want to display an actual icon if you are using a merged icon fonts (see docs/FONTS.md)
static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}