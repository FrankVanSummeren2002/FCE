#include "Core/FCEpch.h"
#include "Maps/Level.h"
#include "LDtkLoader/World.hpp"
#include "Core/Engine.h"
#include "maps/World.h"
using namespace FCE;
void FCE::Level::ClearLevel()
{
	entt::registry* reg = FCE::Engine::GetRegistery();
	for (auto it : mDeletionEntities)
	{
		reg->destroy(it);
	}
}
void FCE::Level::LoadLevel()
{
	mLevel = &mWorld->GetWorld()->getLevel(mLevelName);
	mLoadFunction(this);
}
Level::Level(const char* levelName, std::function<void(Level*)> LoadEntityFunction,FCE::World* world)
{
	mLevelName = levelName;
	mLoadFunction = LoadEntityFunction;
	mWorld = world;
	LoadLevel();
}

std::vector<EntityData> FCE::Level::LoadEntities(std::string layer, std::string name,bool AddToQueue)
{
	std::vector<EntityData> returnValue;
	std::vector<entt::entity> List;
	if (!hasLayer(layer))
		return returnValue;
	const ldtk::Layer& entityLayer = mLevel->getLayer(layer);
	uint32_t entityLayerCellSize = entityLayer.getCellSize();
	uint32_t gridHeight = entityLayer.getGridSize().y;

	const std::vector<std::reference_wrapper<ldtk::Entity>>& entity = entityLayer.getEntitiesByName(name);

	for (uint32_t i = 0; i < entity.size(); i++)
	{
		EntityData data;
		ldtk::IntPoint entityPositionIntPoint = entity[i].get().getGridPosition();
		const ldtk::IntPoint& entitySize = entity[i].get().getSize();
		
		float x = entityPositionIntPoint.x * entityLayerCellSize;
		float y = entityPositionIntPoint.y * entityLayerCellSize;

		data.Entity = FCE::Engine::GetRegistery()->create();
		List.push_back(data.Entity);
		data.Pos = glm::vec3(x,y,0);
		data.Size = glm::vec2(entitySize.x, entitySize.y);

		returnValue.push_back(data);
	}

	if (AddToQueue)
		mDeletionEntities.insert(mDeletionEntities.begin(),List.begin(),List.end());

	return returnValue;
}

bool FCE::Level::hasLayer(std::string Layer)
{
	int layer = -1;
	const auto& layers = mLevel->allLayers();
	std::string name;
	for (int i = 0; i < layers.size(); i++)
	{
		if (layers[i].getName() == Layer)
		{
			layer = i;
			name = layers[i].getName();
		}
	}

	if (layer == -1)
		return false;

	return true;
}

std::vector<GridValues> FCE::Level::LoadFromTileSheet(std::string layer)
{
	std::vector<GridValues> returnValue;
	if (!hasLayer(layer))
		return returnValue;
	const ldtk::Layer& entityLayer = mLevel->getLayer(layer);
	uint32_t entityLayerCellSize = entityLayer.getCellSize();
	uint32_t gridHeight = entityLayer.getGridSize().y;
	auto Tiles = entityLayer.allTiles();
	for (const auto& it : Tiles)
	{
		GridValues value;
		value.index = -1;
		
		auto NewLocation = it.position;
		bool Resized = false;
		for (auto Resize : returnValue)
		{
			if (Resize.index == it.tileId)
			{
				auto Pos = Resize.GridPos + Resize.Size / glm::vec2(entityLayerCellSize);
				if (Pos.x == NewLocation.x && Resize.GridPos.y == NewLocation.y)
				{
					Resize.Size.x += entityLayerCellSize;
					Resized = true;
				}
				else if (Pos.y == NewLocation.y && Resize.GridPos.x == NewLocation.x)
				{
					Resize.Size.y += entityLayerCellSize;
					Resized = true;
				}
			}
		}
		if (!Resized)
		{
			value.GridPos.x = it.position.x;
			value.GridPos.y = it.position.y;
			value.index = it.tileId;
			value.Pos.x = it.world_position.x;
			value.Pos.y = it.world_position.y;
			value.TileMap.x = it.texture_position.x;
			value.TileMap.y = it.texture_position.y;
			value.Pos.z = 0;
			value.Size = glm::vec2(entityLayerCellSize, entityLayerCellSize);
			returnValue.push_back(value);
		}
	}
	return returnValue;
}

std::vector<EntityData> FCE::Level::LoadEntities(std::string layer, std::string name, bool AddToQueue, std::vector<std::reference_wrapper<ldtk::Entity>>& entity)
{
	std::vector<EntityData> returnValue;
	std::vector<entt::entity> List;
	if (!hasLayer(layer))
		return returnValue;
	const ldtk::Layer& entityLayer = mLevel->getLayer(layer);
	uint32_t entityLayerCellSize = entityLayer.getCellSize();
	uint32_t gridHeight = entityLayer.getGridSize().y;

	entity = entityLayer.getEntitiesByName(name);

	for (uint32_t i = 0; i < entity.size(); i++)
	{
		EntityData data;
		ldtk::IntPoint entityPositionIntPoint = entity[i].get().getGridPosition();
		const ldtk::IntPoint& entitySize = entity[i].get().getSize();

		float x = entityPositionIntPoint.x * entityLayerCellSize;
		float y = entityPositionIntPoint.y * entityLayerCellSize;

		data.Entity = FCE::Engine::GetRegistery()->create();
		List.push_back(data.Entity);
		data.Pos = glm::vec3(x, y, 0);
		data.Size = glm::vec2(entitySize.x, entitySize.y);

		returnValue.push_back(data);
	}

	if (AddToQueue)
		mDeletionEntities.insert(mDeletionEntities.begin(), List.begin(), List.end());

	return returnValue;
}

std::vector<GridValues> FCE::Level::LoadGridTiles(std::string layer)
{
	std::vector<GridValues> returnValue;
	if (!hasLayer(layer))
		return returnValue;
	const ldtk::Layer& entityLayer = mLevel->getLayer(layer);
	uint32_t entityLayerCellSize = entityLayer.getCellSize();
	uint32_t gridHeight = entityLayer.getGridSize().y;
	for (int j = 0; j < entityLayer.getGridSize().y; j++)
	{
		for (int i = 0; i < entityLayer.getGridSize().x; i++)
		{
			GridValues value;
			value.index = -1;
			auto it = entityLayer.getIntGridVal(i, j);
			
			//try joining it on an x-line
			if (it.value >= 0)
			{
				if (i > 0)
				{
					auto sec = entityLayer.getIntGridVal(i - 1, j);
					if (sec.value == it.value && returnValue.back().Size.y == entityLayerCellSize && returnValue.back().GridPos.x + (returnValue.back().Size.x / entityLayerCellSize) == i && returnValue.back().GridPos.y == j)
					{
						returnValue.back().Pos.x += static_cast<float>((entityLayerCellSize * 0.5f));
						returnValue.back().Size.x += entityLayerCellSize;
						continue;
					}
				}
				//try joining it on an y-line
				if (j > 0)
				{
					int index = -1;
					int Height = -1;
					for (int z = 0; z < returnValue.size(); z++)
					{
						if (returnValue[z].GridPos.x == i)
						{
							int Reach = returnValue[z].GridPos.y + returnValue[z].Size.y / entityLayerCellSize;
							if (Reach == j && returnValue[z].index == it.value)
							{
								index = z;
								break;
							}
						}
					}
					if (index != -1)
					{
						//if it is already joint on a x-line then dont connect it to a y line
						if (returnValue[index].Size.x == entityLayerCellSize)
						{
							returnValue[index].Pos.y += static_cast<float>((entityLayerCellSize) * 0.5f);
							returnValue[index].Size.y += entityLayerCellSize;
							continue;
						}
					}
				}

				value.GridPos = glm::vec2(i, j);
				value.index = it.value;
				float x = i * entityLayerCellSize;
				//float y = ((gridHeight - j) * entityLayerCellSize) - entityLayerCellSize;
				float y =  j * entityLayerCellSize;
				value.Pos =glm::vec3(x, y, 0);
				value.Size = glm::vec2(entityLayerCellSize, entityLayerCellSize);
				returnValue.push_back(value);
			}
		}
	}
	return returnValue;
}

void FCE::Level::AddEntitiesToDeletionQueue(std::vector<entt::entity> mList)
{
	for (int i = 0; i < mList.size();)
	{
		bool found = false;
		for (int j = 0; j < mDeletionEntities.size(); j++)
		{
			if (mList[i] == mDeletionEntities[j])
			{
				mList.erase(mList.begin() + i);
				found = true;
				break;
			}
		}
		if(!found)
		i++;
	}
	mDeletionEntities.insert(mDeletionEntities.begin(), mList.begin(), mList.end());
}

void FCE::Level::AddEntityToDeletionQueue(entt::entity Entity)
{
		for (int j = 0; j < mDeletionEntities.size(); j++)
		{
			if (Entity == mDeletionEntities[j])
			{
				return;
			}
		}
	mDeletionEntities.push_back(Entity);
}

void FCE::Level::GetSize(int& Width, int& Height)
{
	Width = mLevel->size.x;
	Height = mLevel->size.y;
}

void FCE::Level::RemoveEntityFromDeletionQueue(entt::entity Entity)
{
	for (int i = 0; i < mDeletionEntities.size(); i++)
	{
		entt::entity e = mDeletionEntities[i];
		if (e == Entity)
		{
			mDeletionEntities.erase(mDeletionEntities.begin() + i);
			return;
		}
	}
}

void FCE::Level::Reload()
{
	ClearLevel();
	RequestReload = false;
	mDeletionEntities.clear();
	LoadLevel();
}

#include "Systems/CollisionSystem.h"
Level::~Level()
{
	ClearLevel();
}