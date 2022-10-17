#pragma once
namespace ldtk
{
	class World;
}
namespace FCE
{
	class Level;
	class World
	{
		Level* mLoadedLevel = nullptr;
		ldtk::World* mWorld{ nullptr };
		std::string mFileName;
	public:
		//this will delete the old level and create a new one
		Level* LoadLevel(const char* name, std::function<void(Level*)> callback);
		Level* GetLevel() { return mLoadedLevel; }
		std::string GetFileName() { return mFileName; };
		void LoadWorld(std::string fileName);
		World(std::string fileName);
		void Reload();
		ldtk::World* GetWorld() { return mWorld; };
		void DeleteWorld();
	};
}

