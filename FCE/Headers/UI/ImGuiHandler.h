#pragma once


struct GLFWwindow;
namespace FCE
{
class Inspectable;
class DebugInspectable;

class ImGuiHandler
{
	//draws the main window that contains the game
	static void MainWindow();
public:
	static void Init(GLFWwindow*aWindow);
	static void Update();
	static void AddInspectable(Inspectable* aInspectable);
	static void AddInspectable(DebugInspectable* inspectable);
	static void RemoveInspectable(Inspectable* aInspectable);
	static void RemoveInspectable(DebugInspectable* inspectable);
	static bool IsAWindowHovered();
	static bool GetMainWindowMinMax(glm::vec2& aMin, glm::vec2& aMax);
};

}
