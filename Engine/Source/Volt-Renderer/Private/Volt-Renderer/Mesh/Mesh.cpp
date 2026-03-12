#include "vrpch.h"

#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/Mesh/SubMesh.h"
#include "Volt-Renderer/Material/MaterialTable.h"
#include "Volt-Renderer/Mesh/MeshCommon.h"
#include "Volt-Renderer/RayTracing/RayTracingSceneGeometry.h"
#include "Volt-Renderer/BoundingStructures.h"
#include "Volt-Renderer/Vertex.h"

#include <Volt-Core/Algorithms.h>

#include <RHIModule/RHIFeatures.h>
#include <RHIModule/Buffers/BufferUtility.h>

#include <CoreUtilities/Math/Math.h>
#include <CoreUtilities/Packing.h>

#include <meshoptimizer/meshoptimizer.h>

namespace Volt
{
	namespace Utility
	{
		VT_NODISCARD Vector<uint32_t> ElementCountPrefixSum(const Vector<VertexContainer>& elements)
		{
			Vector<uint32_t> prefixSums(elements.size());

			prefixSums.resize_uninitialized(elements.size());
			prefixSums[0] = 0;

			for (size_t i = 1; i < elements.size(); i++)
			{
				size_t sum = 0;

				for (size_t j = 0; j < i; j++)
				{
					sum += elements.at(j).positions.size();
				}

				prefixSums[i] = static_cast<uint32_t>(sum);
			}

			return prefixSums;
		}
	}

	inline static BoundingSphere GetBoundingSphereFromVertices(const glm::vec3* vertexPtr, const uint32_t* indices, size_t numIndices)
	{
		glm::vec3 minVertex(std::numeric_limits<float>::max());
		glm::vec3 maxVertex(std::numeric_limits<float>::min());

		for (size_t i = 0; i < numIndices; ++i)
		{
			const uint32_t index = indices[i];
			const glm::vec3& vertex = vertexPtr[index];

			minVertex = glm::min(minVertex, vertex);
			maxVertex = glm::max(maxVertex, vertex);
		}

		glm::vec3 extents = (maxVertex - minVertex) * 0.5f;
		glm::vec3 origin = extents + minVertex;

		float radius = 0.f;
		for (size_t i = 0; i < numIndices; ++i)
		{
			const uint32_t index = indices[i];
			const glm::vec3& vertex = vertexPtr[index];

			glm::vec3 offset = vertex - origin;
			float distance = offset.x * offset.x + offset.y * offset.y + offset.z * offset.z;

			radius = std::max(radius, distance);
		}

		radius = std::sqrt(radius);
		return { origin, radius };
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::Initialize(const MeshInitializer& initializer)
	{
		VT_PROFILE_FUNCTION();
		VT_ASSERT_MSG(initializer.IsValid(), "Mesh initializer is not valid!");

		// Copy values
		m_vertexContainer = initializer.GetVertices();
		m_indices = initializer.GetIndices();
		m_subMeshes = initializer.GetSubMeshes();
		m_materialTable = initializer.GetMaterialTable();

		InitializeInternal();
	}

	void Mesh::Serialize(Archive& archive)
	{
		size_t numVertices = m_vertexContainer.Size();

		archive << numVertices;
		archive << m_vertexContainer.positions;

		// Requires manual serialization.
		if (archive.IsLoading())
		{
			m_vertexContainer.materialData.resize_uninitialized(numVertices);
			m_vertexContainer.animationData.resize_uninitialized(numVertices);
		}

		archive.SerializeBytes(m_vertexContainer.materialData.data(), m_vertexContainer.materialData.byte_size());
		archive.SerializeBytes(m_vertexContainer.animationData.data(), m_vertexContainer.animationData.byte_size());

		archive << m_indices;
		archive << m_subMeshes;

		if (archive.IsLoading())
		{
			InitializeInternal();
		}
	}

	void Mesh::SetMaterial(Ref<RenderMaterial> material, uint32_t index)
	{
		m_materialTable.SetMaterial(material, index);
	}

	void Mesh::CreateBoundingSpheres()
	{
		m_subMeshBoundingSpheres.resize(m_subMeshes.size());
		for (uint32_t subMeshIndex = 0; SubMesh& subMesh : m_subMeshes)
		{
			const glm::vec3* positionData = &m_vertexContainer.positions.at(subMesh.vertexStartOffset);
			const uint32_t* indices = &m_indices[subMesh.indexStartOffset];

			BoundingSphere boundingSphere = GetBoundingSphereFromVertices(positionData, indices, subMesh.indexCount);
			m_subMeshBoundingSpheres[subMeshIndex] = boundingSphere;

			subMeshIndex++;
		}
	}

	void Mesh::InitializeInternal()
	{
		VT_PROFILE_FUNCTION();

		const std::string meshName = !m_name.empty() ? m_name + "." : "";

		RHI::BufferUsage bufferRayTracingFlags = RHI::BufferUsage::None;

		if (RHI::RHICanUseRayTracing())
		{
			bufferRayTracingFlags |= RHI::BufferUsage::AccelerationStructureInput | RHI::BufferUsage::DeviceAddress;
		}

		// Index buffer
		{
			const auto& indices = m_indices;

			RHI::BufferDesc desc{};
			desc.numElements = static_cast<uint32_t>(indices.size());
			desc.elementSize = sizeof(uint32_t);
			desc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::IndexBuffer | bufferRayTracingFlags;
			desc.debugName = meshName + "IndexBuffer";

			m_indexBuffer = RHI::Buffer::Create(desc);
			RHI::BufferUtility::StagedBufferUpload(m_indexBuffer, indices.data(), indices.byte_size());
		}

		// Vertex positions
		{
			const auto& vertexPositions = m_vertexContainer.positions;

			RHI::BufferDesc desc{};
			desc.numElements = static_cast<uint32_t>(vertexPositions.size());
			desc.elementSize = sizeof(glm::vec3);
			desc.usage = RHI::BufferUsage::VertexBuffer | bufferRayTracingFlags;
			desc.debugName = meshName + "VertexPositions";

			m_vertexPositionsBuffer = RHI::Buffer::Create(desc);
			RHI::BufferUtility::StagedBufferUpload(m_vertexPositionsBuffer, vertexPositions.data(), vertexPositions.byte_size());
		}

		// Vertex material data
		{
			const auto& vertexMaterialData = m_vertexContainer.materialData;

			RHI::BufferDesc desc{};
			desc.numElements = static_cast<uint32_t>(vertexMaterialData.size());
			desc.elementSize = sizeof(VertexMaterialData);
			desc.debugName = meshName + "VertexMaterialData";
			desc.usage = RHI::BufferUsage::VertexBuffer;

			m_vertexMaterialBuffer = RHI::Buffer::Create(desc);
			RHI::BufferUtility::StagedBufferUpload(m_vertexMaterialBuffer, vertexMaterialData.data(), vertexMaterialData.byte_size());
		}

		// Vertex animation data
		{
			const auto& vertexAnimationData = m_vertexContainer.animationData;

			RHI::BufferDesc desc{};
			desc.numElements = static_cast<uint32_t>(vertexAnimationData.size());
			desc.elementSize = sizeof(VertexAnimationData);
			desc.debugName = meshName + "VertexAnimationData";
			desc.usage = RHI::BufferUsage::VertexBuffer;

			m_vertexAnimationDataBuffer = RHI::Buffer::Create(desc);
			RHI::BufferUtility::StagedBufferUpload(m_vertexAnimationDataBuffer, vertexAnimationData.data(), vertexAnimationData.byte_size());
		}

		CreateBoundingSpheres();

		// Create GPU Meshes
		for (uint32_t i = 0; const auto& subMesh : m_subMeshes)
		{
			auto& gpuMesh = m_gpuMeshes.emplace_back();
			gpuMesh.center = m_subMeshBoundingSpheres[i].center;
			gpuMesh.radius = m_subMeshBoundingSpheres[i].radius;
			gpuMesh.vertexStartOffset = subMesh.vertexStartOffset;
			gpuMesh.indexStartOffset = subMesh.indexStartOffset;

			i++;
		}

		// Create RT data
		if (RHI::RHICanUseRayTracing())
		{
			RayTracingSceneGeometryCreateInfo info{};
			info.indexBuffer = m_indexBuffer;
			info.vertexPositionsBuffer = m_vertexPositionsBuffer;

			for (const auto& subMesh : m_subMeshes)
			{
				auto& geometry = info.geometries.emplace_back();
				geometry.indexCount = subMesh.indexCount;
				geometry.indexOffset = subMesh.indexStartOffset;
				geometry.vertexCount = subMesh.vertexCount;
				geometry.vertexOffset = subMesh.vertexStartOffset;
			}

			m_rayTracingSceneGeometry = RayTracingSceneGeometry::Create(info);
		}

		// Evaluate hash
		for (SubMesh& subMesh : m_subMeshes)
		{
			subMesh.GenerateHash();
		}

		for (const auto& subMesh : m_subMeshes)
		{
			m_hash = Math::HashCombine(m_hash, subMesh.GetHash());
		}
	}

	void MeshInitializer::AddMaterial(Ref<RenderMaterial> material, uint32_t materialIndex)
	{
		m_materialTable.SetMaterial(material, materialIndex);
	}

	void MeshInitializer::AddVertices(const VertexContainer& vertices)
	{
		m_vertices.Append(vertices);
	}

	void MeshInitializer::AddIndices(const Vector<uint32_t>& indices)
	{
		m_indices.append(indices);
	}

	void MeshInitializer::AddSubMesh(const SubMesh& subMesh)
	{
		m_subMeshes.emplace_back(subMesh);
	}

	void MeshInitializer::SetVertices(const Vector<glm::vec3>& vertexPositions, const Vector<VertexMaterialData>& vertexMaterialData, const Vector<VertexAnimationData>& vertexAnimationData)
	{
		m_vertices.positions = vertexPositions;
		m_vertices.materialData = vertexMaterialData;
		m_vertices.animationData = vertexAnimationData;
	}

	void MeshInitializer::SetIndices(const Vector<uint32_t>& indices)
	{
		m_indices = indices;
	}

	void MeshInitializer::SetSubMeshes(const Vector<SubMesh>& subMeshes)
	{
		m_subMeshes = subMeshes;
	}

	void MeshInitializer::SetMaterialTable(const MaterialTable& materialTable)
	{
		m_materialTable = materialTable;
	}

	bool MeshInitializer::IsValid() const
	{
		return !m_subMeshes.empty()
			&& m_vertices.Size() > 0
			&& !m_indices.empty()
			&& m_materialTable.GetSize() > 0;
	}
}
