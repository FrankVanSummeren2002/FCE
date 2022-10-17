#pragma once

namespace FCE
{
	class Level;
	class World;
}

void Load(FCE::Level* level);
entt::entity GetPlayerCamera();
void SetPlayerCamera(entt::entity e);
entt::entity GetPlayer();