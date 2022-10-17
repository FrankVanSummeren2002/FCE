#pragma once
#include "btBulletDynamicsCommon.h"
namespace FCE
{
	class BulletDebugDraw:
		public btIDebugDraw
	{
		int mDebugMode = 0;
	public:

		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color);
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor);
		void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) ;
		void reportErrorWarning(const char* warningString);
		void draw3dText(const btVector3& location, const char* textString);
		void setDebugMode(int debugMode);
		int getDebugMode() const;
		
	};
}

