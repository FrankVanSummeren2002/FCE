#pragma once

namespace FCE
{
	class World;
	class Level;
	//levels will be transitioned at the end of the frame by the engine
	//all you need to do is flip the boolean of LoadLevel
	struct LevelTransition
	{
		bool LoadLevel{ false };

		World* mWorld;
		std::string mLevelPath;
		std::string mWorldPath;
		std::function<void(FCE::Level*)> mLoadFunction;
		LevelTransition(World* world, std::string worldpath, std::string levelpath, std::function<void(FCE::Level*)> loadFunction) :
			mWorld{ world }, mLevelPath{ levelpath }, mWorldPath{ worldpath }, mLoadFunction{ loadFunction }{};
	};
	void DefaultLevelTransition(entt::entity);
}