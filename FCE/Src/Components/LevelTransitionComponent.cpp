#include "Core/FCEpch.h"
#include "Components/LevelTransitionComponent.h"
#include "Core/Engine.h"
#include "Maps/World.h"
void FCE::DefaultLevelTransition(entt::entity ent)
{
  	auto level = FCE::Engine::GetRegistery()->try_get<FCE::LevelTransition>(ent);
	
 	std::string TemporaryWorldString = level->mWorldPath;
	std::string TemporaryLevelString = level->mLevelPath;

	if (std::string(level->mWorld->GetFileName()) != level->mWorldPath)
		level->mWorld->LoadWorld(TemporaryWorldString.c_str());

	level->mWorld->LoadLevel(TemporaryLevelString.c_str(),level->mLoadFunction);

}