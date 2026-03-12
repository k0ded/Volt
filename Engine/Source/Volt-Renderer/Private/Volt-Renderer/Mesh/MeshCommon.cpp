#include "vrpch.h"
#include "Volt-Renderer/Mesh/MeshCommon.h"

#include <CoreUtilities/Packing.h>

namespace Volt
{
	VertexMaterialData VertexMaterialData::Pack(const glm::vec3& normal, const glm::vec4& tangent, const glm::vec2& uv)
	{
		VertexMaterialData result;
		result.normal = Packing::PackNormalToUInt32(normal);
		result.tangent = Packing::EncodeTangent(normal, tangent);
		result.tangentW = tangent.w;
		result.texCoords = glm::packHalf2x16(uv);

		return result;
	}
}
