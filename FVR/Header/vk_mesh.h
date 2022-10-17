#pragma once

#include "vk_types.h"
#include <vec3.hpp>
#include <vec2.hpp>

struct VertexInputDescription 
{

	std::vector<VkVertexInputBindingDescription> bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;

	VkPipelineVertexInputStateCreateFlags flags = 0;
};


struct Vertex 
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 uv;

	static VertexInputDescription GetVertexDescription();
};

struct Mesh 
{
    std::vector<Vertex> mVertices;

	FVR::AllocatedBuffer mVertexBuffer{};

	const char* mFilePath = "";

	bool LoadFromOBJ(const char* filename);
};

struct DebugVertex
{
	glm::vec3 mPos;
	glm::vec4 mColor;

	static VertexInputDescription GetVertexDescription();
};

struct DebugLine
{
	FVR::AllocatedBuffer mVertexBuffer{};
	std::vector<DebugVertex> mPoints;
	glm::mat4 Position{};
};
struct DebugPoint
{
	DebugVertex mPoint;
	glm::vec4 Position2D;
	glm::mat4 Position;
};


struct Vertex2D
{
	glm::vec3 position{};
	glm::vec2 uv{};

	static VertexInputDescription GetVertexDescription();
	Vertex2D() {};
	Vertex2D(glm::vec3 Pos, glm::vec2 TextUv) : position{ Pos }, uv{ TextUv } {};
};

struct TextureMetrics
{
	glm::vec2 mInitialOffset{0};
	glm::vec2 mTextScaling{1};

	bool operator==(TextureMetrics a)
	{
		return mInitialOffset == a.mInitialOffset && mTextScaling == a.mTextScaling;
	}
};

struct Quad
{	
	float mWidth;
	float mHeight;
	TextureMetrics TextureMetrics;
	float mBaseHeight;
	float mBaseWidth;
};