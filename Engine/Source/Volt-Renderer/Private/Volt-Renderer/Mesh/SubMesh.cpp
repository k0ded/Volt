#include "vrpch.h"

#include "Volt-Renderer/Mesh/SubMesh.h"

#include <CoreUtilities/UUID.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt
{
	ArchiveVersionRegistrar g_registerSubMeshArchiveVersion(SubMeshArchiveVersion::guid, SubMeshArchiveVersion::LatestVersion, "SubMeshArchiveVersion");

	void SubMesh::GenerateHash()
	{
		m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(materialIndex));
		m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(vertexCount));
		m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(indexCount));
		m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(vertexStartOffset));
		m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(indexStartOffset));

		using namespace std::chrono;
		auto time = std::chrono::system_clock::now();
		uint64_t count = duration_cast<milliseconds>(time.time_since_epoch()).count();

		m_hash = Math::HashCombine(m_hash, std::hash<uint64_t>()(count));
		m_hash = Math::HashCombine(m_hash, std::hash<uint64_t>()(UUID64()));
	}

	void SubMesh::Serialize(BinaryStreamWriter& streamWriter, const SubMesh& data)
	{
	}

	void SubMesh::Deserialize(BinaryStreamReader& streamReader, SubMesh& outData)
	{
		streamReader.Read(outData.materialIndex);
		streamReader.Read(outData.vertexCount);
		streamReader.Read(outData.indexCount);
		streamReader.Read(outData.vertexStartOffset);
		streamReader.Read(outData.indexStartOffset);
		glm::mat4 transform;
		streamReader.Read(transform);

		glm::quat r;
		glm::vec3 t, s;
		Math::Decompose(transform, t, r, s);

		outData.transform.position = t;
		outData.transform.scale = s;
		outData.transform.rotation = r;

		streamReader.Read(outData.name);
	}

	const bool SubMesh::operator==(const SubMesh& rhs) const
	{
		return m_hash == rhs.m_hash;
	}

	const bool SubMesh::operator!=(const SubMesh& rhs) const
	{
		return m_hash != rhs.m_hash;
	}

	bool operator>(const SubMesh& lhs, const SubMesh& rhs)
	{
		return lhs.m_hash > rhs.m_hash;
	}

	bool operator<(const SubMesh& lhs, const SubMesh& rhs)
	{
		return lhs.m_hash < rhs.m_hash;
	}
}
