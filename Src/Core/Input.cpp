#include "Core/FCEpch.h"
#include "Core/Input.h"
#include "header/VulkanFrontEnd.h"
#include "UI/ImGuiHandler.h"

void KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
}

void FCE::Input::Init(FVR::VulkanFrontEnd* renderer)
{
	mWindow = renderer->GetWindow();
	glfwSetKeyCallback(renderer->GetWindow() , KeyCallBack);
	for (int i = 0; i < GLFW_KEY_LAST; i++)
		mButtons[i] = ButtonStates::UP;

}
void FCE::Input::UpdateKey(int index)
{
	if (mButtons[index] & ButtonStates::PRESSED)
		mButtons[index] &= ~ButtonStates::PRESSED;
	if (mButtons[index] & ButtonStates::RELEASED)
		mButtons[index] &= ~ButtonStates::RELEASED;

}
void FCE::Input::UpdateMouseKey(int index)
{
	if (mMouseButtons[index] & ButtonStates::PRESSED)
		mMouseButtons[index] &= ~ButtonStates::PRESSED;
	if (mMouseButtons[index] & ButtonStates::RELEASED)
		mMouseButtons[index] &= ~ButtonStates::RELEASED;

}
void FCE::Input::Update(float dt)
{
	glfwPollEvents();

	for  (int i = 0; i < GLFW_KEY_LAST; i++)
	{
		UpdateKey(i);
		int state = glfwGetKey(mWindow, i);
		switch (state)
		{
			case GLFW_PRESS:
			{
				if(!(mButtons[i] & ButtonStates::HELD))
				mButtons[i] = (ButtonStates::PRESSED | ButtonStates::HELD) & ~ButtonStates::UP;
				break;
			}
			case GLFW_RELEASE:
			{
				if (!(mButtons[i] & ButtonStates::UP))
				mButtons[i] = ButtonStates::RELEASED | ButtonStates::UP & ~ButtonStates::HELD;
				break;
			}
		default:
			break;
		}
	}

	for (int i = 0; i < GLFW_MOUSE_BUTTON_LAST; i++)
	{
		UpdateMouseKey(i);
		int state = glfwGetMouseButton(mWindow, i);
		switch (state)
		{
		case GLFW_PRESS:
		{
			if (!(mMouseButtons[i] & ButtonStates::HELD))
				mMouseButtons[i] = (ButtonStates::PRESSED | ButtonStates::HELD) & ~ButtonStates::UP;
			break;
		}
		case GLFW_RELEASE:
		{
			if (!(mMouseButtons[i] & ButtonStates::UP))
				mMouseButtons[i] = ButtonStates::RELEASED | ButtonStates::UP & ~ButtonStates::HELD;
			break;
		}
		default:
			break;
		}
	}

	//make a check for holding
}

glm::vec2 FCE::Input::GetMouseScreenPos()
{
	double xpos, ypos;
	glfwGetCursorPos(mWindow, &xpos, &ypos);
	int width, height;
	glfwGetWindowSize(mWindow, &width, &height);
	glm::vec2 min, max;
	FCE::ImGuiHandler::GetMainWindowMinMax(min, max);
	glm::vec2 pos{ xpos,ypos };
	pos -= min * 0.5f;
	pos *= 2.f;
	pos -= (glm::vec2(width, height) * 1.f);

	return pos;
}

bool FCE::Input::IsButton(ButtonStates state,int index)
{
	return state & mButtons[index];
}

bool FCE::Input::IsMouseButton(ButtonStates state, int index)
{
	return state & mMouseButtons[index];
}

