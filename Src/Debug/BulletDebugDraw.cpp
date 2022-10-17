#include "Core/FCEpch.h"
#include "Debug/BulletDebugDraw.h"
#include "Core/Engine.h"
#include "Header/VulkanFrontEnd.h"
#include "Core/Core.h"
void FCE::BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	if(mDebugMode > 0)
	drawLine(from, to, color, color);
}

void FCE::BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
	if (mDebugMode <= 0)
		return;

	glm::vec3 start = bulletToGlm(from);
	glm::vec3 end = bulletToGlm(to);
	glm::vec3 Color = bulletToGlm(fromColor);
	glm::vec3 EndColor = bulletToGlm(toColor);

	Engine::GetRenderer()->DrawLine(start, end, Color, EndColor,!FCE::Engine::GetRenderMode());
}

void FCE::BulletDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
{
	if (mDebugMode > 0)
	FCE::Engine::GetRenderer()->DrawPoint(glm::vec3(PointOnB.m_floats[0], PointOnB.m_floats[1], PointOnB.m_floats[2]), glm::vec3(color[0], color[1], color[2]));
}

void FCE::BulletDebugDraw::reportErrorWarning(const char* warningString)
{
	
	std::cout << "Bullet warning :" << warningString << std::endl;
}

void FCE::BulletDebugDraw::draw3dText(const btVector3& location, const char* textString)
{
}

void FCE::BulletDebugDraw::setDebugMode(int debugMode)
{
	mDebugMode = debugMode;
}

int FCE::BulletDebugDraw::getDebugMode() const
{
	return mDebugMode;
	
}
