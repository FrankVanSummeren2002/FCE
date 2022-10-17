#pragma once

#include "iostream"
#include "entt/entt.hpp"
#include "glm.hpp"
#include <gtc/matrix_transform.hpp>
#include <gtx/transform.hpp>

#include "Core/Engine.h"
#include "Core/FCEpch.h"
#include "Core/Timing.h"
#include "Components/CommonComponents.h"

enum CollisionTypes
{
    COLLISION_TYPE_PLAYER = 0x1 << 0,
    COLLISION_TYPE_WALL = 0x1 << 1,
    COLLISION_TYPE_HITBOX = 0x1 << 2,
    COLLISION_TYPE_ENEMY = 0x1 << 3,
    COLLISION_TYPE_BULLET = 0x1 << 4
};