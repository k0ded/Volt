#pragma once

#include "Volt-Renderer/Mesh/SubMesh.h"
#include "Volt-Renderer/Material/MaterialTable.h"

#include "Volt-Renderer/Vertex.h"
#include "Volt-Renderer/BoundingStructures.h"
#include "Volt-Renderer/Mesh/MeshCommon.h"
#include "Volt-Renderer/GPUScene.h"

#include <RHIModule/Buffers/StorageBuffer.h>

#include <CoreUtilities/Containers/Map.h>

#include <AssetSystem/AssetType.h>

namespace Volt
{
	namespace RHI
	{
		class VertexBuffer;
		class IndexBuffer;
	}

	class RenderMaterial;
	class RayTracingSceneGeometry;

	struct VertexContainer
	{
		Vector<glm::vec3> positions;
		Vector<VertexMaterialData> materialData;
		Vector<VertexAnimationData> animationData;

		VT_INLINE size_t Size() const
		{
			return positions.size();
		}

		VT_INLINE void Resize(size_t size)
		{
			positions.resize(size);
			materialData.resize(size);
			animationData.resize(size);
		}

		VT_INLINE void Append(const VertexContainer& other, const size_t count = 0)
		{
			positions.insert(positions.end(), other.positions.begin(), count > 0 ? other.positions.begin() + count : other.positions.end());
			materialData.insert(materialData.end(), other.materialData.begin(), count > 0 ? other.materialData.begin() + count : other.materialData.end());
			animationData.insert(animationData.end(), other.animationData.begin(), count > 0 ? other.animationData.begin() + count : other.animationData.end());
		}

		VT_INLINE void Add(const glm::vec3& vertexPosition, const VertexMaterialData& vertexMaterialData, const VertexAnimationData& vertexAnimationData)
		{
			positions.push_back(vertexPosition);
			materialData.push_back(vertexMaterialData);
			animationData.push_back(vertexAnimationData);
		}
	};

	class VTR_API MeshInitializer
	{
	public:
		void AddMaterial(Ref<RenderMaterial> material, uint32_t materialIndex);
		void AddVertices(const VertexContainer& vertices);
		void AddIndices(const Vector<uint32_t>& indices);
		void AddSubMesh(const SubMesh& subMesh);

		void SetVertices(const Vector<glm::vec3>& vertexPositions, const Vector<VertexMaterialData>& vertexMaterialData, const Vector<VertexAnimationData>& vertexAnimationData);
		void SetIndices(const Vector<uint32_t>& indices);
		void SetSubMeshes(const Vector<SubMesh>& subMeshes);
		void SetMaterialTable(const MaterialTable& materialTable);

		bool IsValid() const;

		VT_INLINE uint32_t GetNumVertices() const { return static_cast<uint32_t>(m_vertices.Size()); }
		VT_INLINE uint32_t GetNumIndices() const { return static_cast<uint32_t>(m_indices.size()); }
		VT_INLINE const VertexContainer& GetVertices() const { return m_vertices; }
		VT_INLINE const Vector<uint32_t>& GetIndices() const { return m_indices; }
		VT_INLINE const Vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		VT_INLINE const MaterialTable& GetMaterialTable() const { return m_materialTable; }

	private:
		MaterialTable m_materialTable;
		VertexContainer m_vertices;
		Vector<uint32_t> m_indices;
		Vector<SubMesh> m_subMeshes;
	};

	struct MeshStatistics
	{
		uint32_t numVertices;
		uint32_t numPrimitives;
		uint32_t numIndices;
		uint32_t numMaterials;
	};

	class VTR_API Mesh
	{
	public:
		Mesh() = default;
		~Mesh();

		void Initialize(const MeshInitializer& initializer);
		void Serialize(Archive& archive);
		void SetMaterial(Ref<RenderMaterial> material, uint32_t index);

		VT_INLINE void SetName(const std::string& name) { m_name = name; }
		VT_NODISCARD VT_INLINE const std::string& GetName() const { return m_name; }

		inline const Vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		inline Vector<SubMesh>& GetSubMeshesMutable() { return m_subMeshes; }
		inline uint32_t GetNumSubMeshes() const { return static_cast<uint32_t>(m_subMeshes.size()); }

		inline const MaterialTable& GetMaterialTable() const { return m_materialTable; }
		inline const size_t GetVertexCount() const { return m_vertexContainer.Size(); }
		inline const size_t GetIndexCount() const { return m_indices.size(); }

		inline const Vector<uint32_t>& GetIndices() const { return m_indices; }

		inline const BoundingSphere& GetBoundingSphere() const { static BoundingSphere b; return b; }
		inline const Vector<GPUMesh>& GetGPUMeshes() const { return m_gpuMeshes; }

		inline const BoundingSphere& GetSubMeshBoundingSphere(const uint32_t index) const { return m_subMeshBoundingSpheres.at(index);  }

		inline RefPtr<RHI::StorageBuffer> GetVertexPositionsBuffer() const { return m_vertexPositionsBuffer; }
		inline RefPtr<RHI::StorageBuffer> GetVertexMaterialBuffer() const { return m_vertexMaterialBuffer; }
		inline RefPtr<RHI::StorageBuffer> GetVertexAnimationInfoBuffer() const { return m_vertexAnimationDataBuffer; }
		inline RefPtr<RHI::StorageBuffer> GetIndexBuffer() const { return m_indexBuffer; }

		VT_NODISCARD VT_INLINE const VertexContainer& GetVertexContainer() const { return m_vertexContainer; }
		VT_NODISCARD VT_INLINE Ref<RayTracingSceneGeometry> GetRayTracingSceneGeometry() const { return m_rayTracingSceneGeometry; }
		VT_NODISCARD VT_INLINE size_t GetHash() const { return m_hash; }
		
		VT_NODISCARD VT_INLINE bool DoMeshRequireUpdate() const { return m_isDirty; }
		VT_INLINE void ClearStatus() { m_isDirty = false; }

	private:
		friend class MeshSerializer;
		friend class MeshExporterUtilities;
		friend class FbxSourceImporter;
		friend class GLTFSourceImporter;

		void CreateBoundingSpheres();
		void InitializeInternal();

		VertexContainer m_vertexContainer{};
		Vector<uint32_t> m_indices;
		Vector<GPUMesh> m_gpuMeshes;
		Vector<SubMesh> m_subMeshes;

		MaterialTable m_materialTable;

		RefPtr<RHI::StorageBuffer> m_indexBuffer;
		RefPtr<RHI::StorageBuffer> m_vertexPositionsBuffer;
		RefPtr<RHI::StorageBuffer> m_vertexMaterialBuffer;
		RefPtr<RHI::StorageBuffer> m_vertexAnimationDataBuffer;

		Ref<RayTracingSceneGeometry> m_rayTracingSceneGeometry;

		Vector<BoundingSphere> m_subMeshBoundingSpheres;

		bool m_isDirty = false;
		size_t m_hash = 0;
		std::string m_name;
	};
}
