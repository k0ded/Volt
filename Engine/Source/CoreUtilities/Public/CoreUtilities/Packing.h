#pragma once

#include "CoreUtilities/Config.h"

#include <glm/glm.hpp>

namespace Packing
{
	VTCOREUTIL_API extern glm::vec2 OctNormalEncode(glm::vec3 n);
	VTCOREUTIL_API extern glm::vec3 OctNormalDecode(glm::vec2 f);
	VTCOREUTIL_API extern float EncodeTangent(const glm::vec3& normal, const glm::vec3& tangent);
	VTCOREUTIL_API extern glm::vec3 DecodeTangent(const glm::vec3& normal, float packedTangent);
	VTCOREUTIL_API extern uint32_t PackNormalToUInt32(const glm::vec3& normal);
	VTCOREUTIL_API extern glm::vec3 UnpackNormalFromUInt32(uint32_t packedNormal);
}
