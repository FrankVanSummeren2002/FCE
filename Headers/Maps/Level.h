#pragma once
namespace ldtk
{
	class World;
	class Level;
	class Entity;
	class Layer;
}
namespace FCE
{
	class World;

	struct EntityData
	{
		entt::entity Entity{};
		glm::vec3 Pos{};
		glm::vec2 Size{};
		EntityData(entt::entity E, glm::vec3 P, glm::vec2 S) :Entity{ E }, Pos{ P }, Size{S} {};
		EntityData() {};
	};

	struct GridValues
	{
		glm::vec3 Pos;
		glm::vec2 GridPos;
		glm::vec2 Size;
		glm::vec2 TileMap;
		int index;
	};

	class Level
	{
		std::function<void(Level*)> mLoadFunction;
		std::vector<entt::entity> mDeletionEntities;

		std::string mLevelName;
		const ldtk::Level* mLevel;
		World* mWorld;
		void ClearLevel();
		void LoadLevel();
		bool hasLayer(std::string Layer);

	public:
		bool ShouldBeDeleted{ false };
		bool RequestReload{ false };

		Level(const char* levelName, std::function<void(Level*)> LoadEntityFunction,World* world);
		//these entities are added to the deletion queue list if the boolean is true
		std::vector<EntityData> LoadEntities(std::string layer, std::string Name,bool AddToDeletionQueue);
		std::vector<GridValues> LoadFromTileSheet(std::string layer);
		std::vector<EntityData> LoadEntities(std::string layer, std::string name, bool AddToQueue,
			std::vector<std::reference_wrapper<ldtk::Entity>>& entity);
		std::vector<GridValues> LoadGridTiles(std::string layer);

		void AddEntitiesToDeletionQueue(std::vector<entt::entity> mList);
		void AddEntityToDeletionQueue(entt::entity Entity);
		void RemoveEntityFromDeletionQueue(entt::entity Entity);

		void Reload();

		void GetSize(int& width, int& height);

		const char* GetFileName() { return mLevelName.c_str(); }
		std::function<void(Level*)> GetLoadFunction() { return mLoadFunction; };
		
		World* GetWorld() { return mWorld; };
		~Level();
	};
}

