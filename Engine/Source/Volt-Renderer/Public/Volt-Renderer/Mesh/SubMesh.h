#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/GPUScene.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Volt
{
	struct SubMeshArchiveVersion
	{
		enum Type
		{
			BaseVersion = 0,

			// Switched from mat4 to TRS for transform storage.
			UseTRSAsTransform = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{116C57BF-9AE5-4445-A20C-A089D247232B}"_guid;

	private:
		SubMeshArchiveVersion() {}
	};

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

		GPUTransform transform;
		std::string name;

		VT_INLINE friend Archive& operator<<(Archive& archive, SubMesh& value)
		{
			archive.UseVersion(SubMeshArchiveVersion::guid);

			const int32_t currentVersion = archive.GetVersion(SubMeshArchiveVersion::guid);

			archive << value.materialIndex;
			archive << value.vertexCount;
			archive << value.indexCount;
			archive << value.vertexStartOffset;
			archive << value.indexStartOffset;

			if (archive.IsLoading() && currentVersion < SubMeshArchiveVersion::UseTRSAsTransform)
			{
				glm::mat4 transform;
				archive << transform;

				glm::vec3 t, s;
				glm::quat r;
				Math::Decompose(transform, t, r, s);

				value.transform.position = t;
				value.transform.scale = s;
				value.transform.rotation = r;
			}
			else
			{
				archive << value.transform.rotation;
				archive << value.transform.position;
				archive << value.transform.scale;
			}

			archive << value.name;

			return archive;
		}

	private:
		size_t m_hash = 0;
	};
}
