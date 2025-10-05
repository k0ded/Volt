#pragma once

#include "Volt-Renderer/Config.h"

#include <cstdint>
#include <glm/glm.hpp>

class BinaryStreamWriter;
class BinaryStreamReader;

namespace Volt
{
	struct VTR_API SubMesh
	{
		SubMesh() = default;

		void GenerateHash();

		inline const size_t GetHash() const { return m_hash; }

		const bool operator==(const SubMesh& rhs) const;
		const bool operator!=(const SubMesh& rhs) const;

		friend bool operator>(const SubMesh& lhs, const SubMesh& rhs);
		friend bool operator<(const SubMesh& lhs, const SubMesh& rhs);

		uint32_t materialIndex = 0;
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		uint32_t vertexStartOffset = 0;
		uint32_t indexStartOffset = 0;

		glm::mat4 transform = { 1.f };
		std::string name;

		static void Serialize(BinaryStreamWriter& streamWriter, const SubMesh& data);
		static void Deserialize(BinaryStreamReader& streamReader, SubMesh& outData);

	private:
		size_t m_hash = 0;
	};
}
