#pragma once


namespace FVR
{
	class VulkanFrontEnd;
}
namespace FCE
{
	class Input;
	class World;
	class BasicEngineUtilities;
	class Engine
	{
		static void Init();
		//will choose wheter to use the debugCamera or normal Camera
		static entt::entity ChooseCamera();
	public:

		//returns true if it is rendering 3D
	
		static bool Update(float DT);
		static void Run(std::function<void()>GameInit, std::function<bool(float DT)>GameLoop, std::function<void()>CleanUp);

		static void SetRenderMode(bool render3D);
		static void SetActiveCameraHandle(entt::entity CameraComponent);
		static void SetActiveWorld(World* world);

		static entt::registry* GetRegistery();
		//returns true if it is rendering in 3d
		static bool GetRenderMode();
		static bool GetDebugDrawing();
		static BasicEngineUtilities* GetBasicEngineUtilities();
		static entt::entity GetActiveCameraHandle();
		static FCE::Input* GetInputHandler();
		static FVR::VulkanFrontEnd* GetRenderer();
		static World* GetActiveWorld();
	
	

		static bool IsDebugging();
	};
}

