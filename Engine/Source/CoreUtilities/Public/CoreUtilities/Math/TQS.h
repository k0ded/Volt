#pragma once

#include <glm/glm.hpp>

struct TQS
{
	glm::vec3 translation = 0.f;
	glm::quat rotation = glm::identity<glm::quat>();
	glm::vec3 scale = 1.f;

	glm::vec3 Transform(const glm::vec3& inPosition) const
	{
		return glm::rotate(rotation, inPosition * scale) + translation;
	}

	glm::vec3 Rotate(const glm::vec3 inVector) const
	{
		return glm::rotate(rotation, inVector);
	}

	static TQS Combine(const TQS& lhs, const TQS& rhs)
	{
		TQS result;
		result.scale = lhs.scale * rhs.scale;
		result.rotation = lhs.rotation * rhs.rotation;
		result.translation = glm::rotate(lhs.rotation, rhs.translation * lhs.scale) + lhs.translation;
	
		return result;
	}

	static TQS Make(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale)
	{
		TQS result;
		result.translation = translation;
		result.rotation = rotation;
		result.scale = scale;

		return result;
	}

	static TQS Make(const glm::vec3& translation, const glm::vec3& eulerAngles, const glm::vec3& scale)
	{
		return Make(translation, glm::quat(eulerAngles), scale);
	}
};
