#include "Core/FCEpch.h"
#include "Maps/World.h"
#include "Maps/Level.h"
#include "LDtkLoader/World.hpp"
FCE::Level* FCE::World::LoadLevel(const char* name,std::function<void(Level*)> callback)
{
	std::function<void(Level*)> testing = callback;
	if (mLoadedLevel)
	{
		delete mLoadedLevel;
	}
	
	mLoadedLevel = new Level(name,testing,this);

	return mLoadedLevel;
}

void FCE::World::LoadWorld(std::string fileName)
{
	if (mWorld)
		delete mWorld;

	mWorld = new ldtk::World();
	mWorld->loadFromFile(fileName.c_str());
	mFileName = fileName;
}


FCE::World::World(std::string fileName)
{
	LoadWorld(fileName);
}

void FCE::World::Reload()
{
	LoadWorld(mFileName.c_str());
	mLoadedLevel->Reload();
}

void FCE::World::DeleteWorld()
{
	if (mLoadedLevel)
		delete mLoadedLevel;

	delete mWorld;

	delete this;
}
