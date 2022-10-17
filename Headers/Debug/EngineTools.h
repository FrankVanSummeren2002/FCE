#pragma once
#include "UI/DebugInspectable.h"
namespace FCE
{
    class Level;
    class BasicEngineUtilities :
        public FCE::DebugInspectable
    {
        std::unordered_map<const char*, std::function<void(Level*)>> mLevelLoadFunctions;
        int currentItem;
        std::vector<const char*> SelectedLoadFunction;
        std::string ChosenWorld;
        std::string ChosenLevel;
    public:
        bool Reload = false;
        bool LoadWorld = false;
        bool KeepPlayerPos = false;
        bool mPaused = false;
        bool mSingleFrameCalled = false;
        bool mIsDebugCamera = false;
        float mDebugCameraMultiplier{ 1 };
        float mCameraMovement{ 100.f };
        entt::entity mDebugCamera;

        BasicEngineUtilities();

        void Display();
        void RegisterLoadFunction(const char* name, std::function<void(Level*)> loadFunction);
        void DebugCameraInput(float DT);
        void CheckWorldUpdates();
    };

    class BasicRenderUtilities:
        public FCE::DebugInspectable
    {
    public:
        bool mRenderDebug = false;

        BasicRenderUtilities();

        void Display();
    };
}
