#include "cupch.h"
#include "CoreUtilities/Packing.h"
#include "CoreUtilities/CompilerTraits.h"

namespace Packing
{
	VT_INLINE glm::vec2 OctNormalWrap(glm::vec2 v)
	{
		glm::vec2 wrap;
		wrap.x = (1.0f - glm::abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f);
		wrap.y = (1.0f - glm::abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f);
		return wrap;
	}

	glm::vec2 OctNormalEncode(glm::vec3 n)
	{
		n /= (glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z));

		glm::vec2 wrapped = OctNormalWrap(n);

		glm::vec2 result;
		result.x = n.z >= 0.0f ? n.x : wrapped.x;
		result.y = n.z >= 0.0f ? n.y : wrapped.y;

		result.x = result.x * 0.5f + 0.5f;
		result.y = result.y * 0.5f + 0.5f;

		return result;
	}

	glm::vec3 OctNormalDecode(glm::vec2 f)
	{
		f = f * 2.f - 1.f;

		// https://twitter.com/Stubbesaurus/status/937994790553227264
		glm::vec3 n = glm::vec3(f.x, f.y, 1.f - abs(f.x) - abs(f.y));
		float t = glm::clamp(-n.z, 0.f, 1.f);

		n.x += n.x >= 0.0f ? -t : t;
		n.y += n.y >= 0.0f ? -t : t;

		return normalize(n);
	}

	// From https://www.jeremyong.com/graphics/2023/01/09/tangent-spaces-and-diamond-encoding/
	VT_INLINE float DiamondEncode(const glm::vec2& p)
	{
		// Project to the unit diamond, then to the x-axis.
		float x = p.x / (glm::abs(p.x) + glm::abs(p.y));

		// Contract the x coordinate by a factor of 4 to represent all 4 quadrants in
		// the unit range and remap
		float pySign = 0.f;
		if (p.y < 0.f)
		{
			pySign = -1.f;
		}
		else if (p.y > 0.f)
		{
			pySign = 1.f;
		}

		return -pySign * 0.25f * x + 0.5f + pySign * 0.25f;
	}

	// Given a normal and tangent vector, encode the tangent as a single float that can be
	// subsequently quantized.
	float EncodeTangent(const glm::vec3& normal, const glm::vec3& tangent)
	{
		// First, find a canonical direction in the tangent plane
		glm::vec3 t1;
		if (abs(normal.y) > abs(normal.z))
		{
			// Pick a canonical direction orthogonal to n with z = 0
			t1 = glm::vec3(normal.y, -normal.x, 0.f);
		}
		else
		{
			// Pick a canonical direction orthogonal to n with y = 0
			t1 = glm::vec3(normal.z, 0.f, -normal.x);
		}
		t1 = normalize(t1);

		// Construct t2 such that t1 and t2 span the plane
		glm::vec3 t2 = cross(t1, normal);

		// Decompose the tangent into two coordinates in the canonical basis
		glm::vec2 packed_tangent = glm::vec2(dot(tangent, t1), dot(tangent, t2));

		// Apply our diamond encoding to our two coordinates
		return DiamondEncode(packed_tangent);
	}

	glm::vec2 DiamondDecode(float p)
	{
		glm::vec2 v;

		// Remap p to the appropriate segment on the diamond
		float p_sign = glm::sign(p - 0.5f);
		v.x = -p_sign * 4.f * p + 1.f + p_sign * 2.f;
		v.y = p_sign * (1.f - glm::abs(v.x));

		// Normalization extends the point on the diamond back to the unit circle
		return glm::normalize(v);
	}

	glm::vec3 DecodeTangent(const glm::vec3& normal, float packedTangent)
	{
		// As in the encode step, find our canonical tangent basis span(t1, t2)
		glm::vec3 t1;
		if (glm::abs(normal.y) > glm::abs(normal.z))
		{
			t1 = glm::vec3(normal.y, -normal.x, 0.f);
		}
		else
		{
			t1 = glm::vec3(normal.z, 0.f, -normal.x);
		}
		t1 = glm::normalize(t1);

		glm::vec3 t2 = glm::cross(t1, normal);

		// Recover the coordinates used with t1 and t2
		glm::vec2 packed_tangent = DiamondDecode(packedTangent);

		return packed_tangent.x * t1 + packed_tangent.y * t2;
	}

	uint32_t PackNormalToUInt32(const glm::vec3& normal)
	{
		const glm::vec2 octNormal = OctNormalEncode(normal);
		glm::uvec2 quantizedOctNormal = glm::clamp(glm::uvec2(octNormal.x * 65535.f, octNormal.y * 65535.f), glm::uvec2(0), glm::uvec2(65535));
		return ((quantizedOctNormal.x & 0xFFFF) << 16) | (quantizedOctNormal.y & 0xFFFF);
	}

	glm::vec3 UnpackNormalFromUInt32(uint32_t packedNormal)
	{
		glm::vec2 quantizedOctahedron = glm::vec2((packedNormal >> 16) & 0xFFFF, packedNormal & 0xFFFF);
		glm::vec2 worldNormalAsOctahedron = ((quantizedOctahedron / 65535.f) - 0.5f) * 2.0f;
		glm::vec3 worldNormal = OctNormalDecode(worldNormalAsOctahedron);

		return worldNormal;
	}
}
