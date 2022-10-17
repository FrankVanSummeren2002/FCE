#pragma once
#include "imgui-docking/imgui.h"
#define IMGUI_LEFT_LABEL(func, label, ...) (ImGui::TextUnformatted(label), ImGui::SameLine(), func("##" label, __VA_ARGS__))
//Inheriting from this class makes it so that it will be displayed trough the imgui windows
//It will have its own window and will automaticly be added to the imgui handler
//You need to set the name inside of the constructor of your own class

namespace FCE
{
	class DebugInspectable
	{
		const char* mName = "";

	public:
		//wheter this should be hidden from the imgui handler
		bool mHidden = false;
		//wheter this window should be open
		bool mOpen = false;
		//when you turn this to true the default imgui begin / end will not be called in the
		//imgui handler so you can call it in display instead
		bool mOverideBeginEnd = false;
		//The imgui window flags
		ImGuiWindowFlags mWindowFlags;

		//function to display the imgui window
		//imgui begin and end will be called by the handler unless you set the mOverideBeginEnd to true
		virtual void Display();

		//set the name of the window It has to be set once and has to be unique
		void SetName(const char* aName);
		const char* GetName();

		//function to add the current Inspectable to the Handler
		//should be automaticly done by the constructor
		void AddToHandler();
		DebugInspectable();

		virtual ~DebugInspectable();
	};
}