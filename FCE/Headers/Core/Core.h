#pragma once

namespace FCE
{
	//sorts the two vectors in one vector with the smallest xyz and one with the biggest xyz
	inline void MinMaxVector(glm::vec3& min, glm::vec3& max)
	{
		float temp;
		if (min.x > max.x)
		{
			temp = min.x;
			min.x = max.x;
			max.x = temp;
		}

		if (min.y > max.y)
		{
			temp = min.y;
			min.y = max.y;
			max.y = temp;
		}

		if (min.z > max.z)
		{
			temp = min.z;
			min.z = max.z;
			max.z = temp;
		}
	}

	inline float Lerp(float A, float B, float Alpha)
	{
		return A + (B - A) * Alpha;
	}

	inline void Rotate2D(glm::vec2& Dir, float angle)
	{
		float cs = cos(angle);
		float sn = sin(angle);
		float x = Dir.x;
		float y = Dir.y;
		Dir.x = x * cs - y * sn;
		Dir.y = x * sn + y * cs;
	}

	inline void Rotate2D(glm::vec3& Dir, float angle)
	{
		float cs = cos(angle);
		float sn = sin(angle);
		float x = Dir.x;
		float y = Dir.y;
		Dir.x = x * cs - y * sn;
		Dir.y = x * sn + y * cs;
	}

	inline glm::vec3 Refract(glm::vec3 Incoming, glm::vec3 Normal)
	{
		return Incoming - 2 * glm::dot(Incoming, Normal) * Normal;
	}


	//Thanks to https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=11462
	inline glm::vec3 bulletToGlm(const btVector3& v) { return glm::vec3(v.getX(), v.getY(), v.getZ()); }

	inline btVector3 glmToBullet(const glm::vec3& v) { return btVector3(v.x, v.y, v.z); }

	inline glm::mat4 bulletToGlm(const btQuaternion& q)
	{
		glm::quat Quat = glm::quat((float)q.getX(), (float)q.getY(), (float)q.getZ(), (float)q.getW());
		return glm::toMat4(Quat);
	}

	inline btQuaternion glmToBullet(const glm::quat& q) { return btQuaternion(q.x, q.y, q.z, q.w); }

	inline btQuaternion glmToBullet(const glm::mat4& m)
	{
		glm::quat q = glm::toQuat(m);
		return	glmToBullet(q);
	}

	inline btMatrix3x3 glmToBullet(const glm::mat3& m) { return btMatrix3x3(m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2]); }

	inline btTransform glmToBullet(glm::vec3 transform, glm::mat4 rotation)
	{
		btTransform trans;
		trans.setOrigin(glmToBullet(transform));
		//trans.setRotation(glmToBullet(rotation));
		return trans;
	}
	
}