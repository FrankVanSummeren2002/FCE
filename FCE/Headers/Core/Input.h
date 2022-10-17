#pragma once
#include "GLFW/glfw3.h"
namespace FVR
{
	class VulkanFrontEnd;
}
namespace FCE
{
	enum ButtonStates
	{
		PRESSED =0x1,
		RELEASED = 0x2,
		HELD = 0x4,
		UP = 0x8
	};
	class Input
	{
		GLFWwindow* mWindow;
		uint32_t mButtons[GLFW_KEY_LAST];
		uint32_t mMouseButtons[GLFW_MOUSE_BUTTON_LAST];

		void UpdateKey(int index);
		void UpdateMouseKey(int index);

	public:
		void Init(FVR::VulkanFrontEnd* renderer);

		void Update(float dt);
		glm::vec2 GetMouseScreenPos();
		bool IsButton(ButtonStates state, int index);
		bool IsMouseButton(ButtonStates state, int index);
	};
}

