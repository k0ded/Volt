#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/Resources/GrowingGPUBuffer.h>

#include <CoreUtilities/Containers/Map.h>

#include <span>

VT_DECLARE_LOG_CATEGORY(LogRenderScene, LogVerbosity::Trace);

namespace Volt
{
	namespace RHI
	{
		class BufferView;
		class StorageBuffer;
	}

	class EntityScene;
	class RenderMaterial;
	class RenderGraph;
	class MotionWeaver;
	class RayTracingScene;

	struct SceneLightDescription;

	struct GPUSceneBuffers
	{
		Ref<GrowingGPUBuffer> meshesBuffer;
		Ref<GrowingGPUBuffer> materialsBuffer;
		Ref<GrowingGPUBuffer> primitiveDrawDataBuffer;
		Ref<GrowingGPUBuffer> prevPrimitiveDrawDataBuffer;
		Ref<GrowingGPUBuffer> bonesBuffer;
		Ref<GrowingGPUBuffer> lightsBuffer;

		Ref<GrowingGPUBuffer> validPrimitiveDrawDatasBuffer;
		RGBufferRef perMeshIndirectDrawCommands;
	};

	class VTR_API RenderScene
	{
	public:
		RenderScene(EntityScene* sceneRef);
		~RenderScene();

		void Update(RenderGraph& renderGraph);
		void EndFrame(RenderGraph& renderGraph);

		void InvalidatePrimitiveInstance(UUID64 renderObject);
		void InvalidateMesh(Ref<Mesh> mesh);
		void InvalidateMaterial(Ref<RenderMaterial> material);

		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<MotionWeaver> motionWeaver, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		void RemovePrimitiveInstance(UUID64 id);

		void InvalidateLightInstance(UUID64 id);

		UUID64 AddLightInstance(EntityID entityId, const SceneLightDescription& description);
		void RemoveLightInstance(UUID64 id);

		VT_INLINE VT_NODISCARD uint32_t GetNumRenderPrimitives() const { return static_cast<uint32_t>(m_renderPrimitives.size()); }
		VT_INLINE VT_NODISCARD uint32_t GetIndividualMeshCount() const { return m_currentIndividualMeshCount; }
		VT_INLINE VT_NODISCARD uint32_t GetIndividualMaterialCount() const { return static_cast<uint32_t>(m_individualMaterials.size()); }
		VT_INLINE VT_NODISCARD uint32_t GetMeshletCount() const { return m_currentMeshletCount; }
		VT_INLINE VT_NODISCARD uint32_t GetDrawCount() const { return m_renderPrimitives.empty() ? 0u : static_cast<uint32_t>(m_primitiveDrawData.size()); }
		VT_INLINE VT_NODISCARD uint32_t GetLightCount() const { return static_cast<uint32_t>(m_renderLights.size()); }
		VT_INLINE VT_NODISCARD size_t GetMaxPrimitiveIndex() const { return m_primitiveIndicesContainer.GetMaxIndex(); }

		VT_NODISCARD Weak<RenderMaterial> GetMaterialFromID(const uint32_t materialId) const;

		VT_NODISCARD const uint32_t GetMeshID(Weak<Mesh> mesh, uint32_t subMeshIndex) const;
		VT_NODISCARD const uint32_t GetMaterialIndex(Weak<RenderMaterial> material) const;
		VT_NODISCARD const uint32_t GetMeshIndex(Weak<Mesh> mesh) const;
		VT_NODISCARD const uint32_t GetPrimitiveIndexFromID(UUID64 primitiveId) const;

		VT_INLINE VT_NODISCARD const GPUSceneBuffers GetGPUSceneBuffers() const { return m_buffers; }
		VT_NODISCARD GPUSceneParameters GetGPUSceneParameters(RenderGraph& renderGraph) const;

		VT_NODISCARD Vector<RenderPrimitiveData>::iterator begin() { return m_renderPrimitives.begin(); }
		VT_NODISCARD Vector<RenderPrimitiveData>::iterator end() { return m_renderPrimitives.end(); }

		VT_NODISCARD VT_INLINE const Vector<RenderLightData>& GetRenderLightData() const { return m_renderLights; }
		VT_NODISCARD VT_INLINE const Vector<RenderPrimitiveData>& GetRenderPrimitives() const { return m_renderPrimitives; }

		VT_NODISCARD const Vector<RenderPrimitiveData>::const_iterator cbegin() const { return m_renderPrimitives.cbegin(); }
		VT_NODISCARD const Vector<RenderPrimitiveData>::const_iterator cend() const { return m_renderPrimitives.cend(); }

		VT_NODISCARD const RenderPrimitiveData& GetPrimitiveDataFromID(UUID64 id) const;
		VT_NODISCARD const RenderLightData& GetLightDataFromID(UUID64 id) const;
		VT_NODISCARD Vector<uint32_t> GetPrimitiveIndicesFromEntityID(EntityID entityId) const;

		VT_NODISCARD VT_INLINE std::span<const GPUMesh> GetGPUMeshes() const { return m_gpuMeshes; }
		VT_NODISCARD VT_INLINE std::span<const PrimitiveDrawData> GetPrimitiveDrawData() const { return m_primitiveDrawData; }
		VT_NODISCARD VT_INLINE Ref<RayTracingScene> GetRayTracingScene() const { return m_rayTracingScene; }

	private:
		void BuildGPUMaterial(Weak<RenderMaterial> material, GPUMaterial& gpuMaterial);

		void BuildSinglePrimitiveDrawData(PrimitiveDrawData& primitiveDrawData, const RenderPrimitiveData& renderPrimitive);
		void BuildSingleLightDrawData(LightDrawData& lightDrawData, RenderLightData& renderLight);

		void TryAddMesh(Ref<Mesh> mesh);
		void TryAddMaterial(Ref<RenderMaterial> material);

		void UpdateInvalidMaterials(RenderGraph& renderGraph);
		void UpdateInvalidMeshes(RenderGraph& renderGraph);
		void UpdateInvalidPrimitiveData(RenderGraph& renderGraph);
		void CompactValidPrimitiveDrawDatas(RenderGraph& renderGraph);
		void BuildPerMeshIndirectDrawCommands(RenderGraph& renderGraph);

		void UpdateInvalidLights(RenderGraph& renderGraph);

		VT_NODISCARD RenderLightData& GetLightDataFromID(UUID64 id);
		VT_NODISCARD PrimitiveDrawData& GetPrimitiveDrawDataFromIndex(size_t index);

		struct InvalidMaterial
		{
			Weak<RenderMaterial> material;
			size_t index;
		};

		struct InvalidMesh
		{
			Weak<Mesh> mesh;
			uint32_t subMeshIndex;
			size_t index;
		};

		struct InvalidDrawData
		{
			UUID64 id;
			size_t index;
		};

		struct MeshAndSubMeshIndex
		{
			size_t meshIndex;
			uint32_t subMeshIndex;
		};

		class PrimitiveIndicesContainer
		{
		public:
			size_t GetAvailableIndex(UUID64 id);
			void FreeIndexWithID(UUID64 id);
			void InvalidateIndexWithID(UUID64 id);

			VT_INLINE size_t GetIndexFromID(UUID64 id) const { return m_primitiveIndexFromPrimitiveID.at(id); }
			VT_INLINE size_t GetMaxIndex() const { return m_nextIndex; }

			VT_INLINE Vector<size_t> GetAndClearRemovedIndices() 
			{ 
				Vector<size_t> tempVector = m_removedPrimitiveDataIndices; 
				m_removedPrimitiveDataIndices.clear(); 
				return tempVector; 
			}

			VT_INLINE Vector<InvalidDrawData> GetAndClearInvalidIndices() 
			{ 
				Vector<InvalidDrawData> tempVector = m_invalidPrimitiveDataIndices;
				m_invalidPrimitiveDataIndices.clear();
				return tempVector; 
			}

		private:
			Vector<size_t> m_removedPrimitiveDataIndices;
			Vector<size_t> m_freePrimitiveDataIndices;

			Vector<InvalidDrawData> m_invalidPrimitiveDataIndices;
			Map<UUID64, size_t> m_primitiveIndexFromPrimitiveID;

			size_t m_nextIndex = 0;
		};

		Ref<RayTracingScene> m_rayTracingScene;

		Vector<UUID64> m_animatedRenderObjects;
		Vector<RenderPrimitiveData> m_renderPrimitives;
		Vector<RenderLightData> m_renderLights;

		Vector<GPUMesh> m_gpuMeshes;
		Vector<GPUMaterial> m_gpuMaterials;

		Vector<InvalidMaterial> m_invalidMaterials;
		Vector<InvalidMesh> m_invalidMeshes;

		Map<size_t, size_t> m_gpuMaterialIndexFromMaterialHash;
		Map<size_t, uint32_t> m_meshSubMeshToGPUMeshIndex;
		Map<uint32_t, MeshAndSubMeshIndex> m_gpuMeshIndexToMeshAndSubMeshIndex;

		Vector<Weak<Mesh>> m_individualMeshes;
		Vector<Weak<RenderMaterial>> m_individualMaterials;
		Vector<glm::mat4> m_animationBufferStorage;
		std::mutex m_materialUpdateMutex;
		std::mutex m_meshUpdateMutex;

		// Scene Primitives
		Vector<PrimitiveDrawData> m_primitiveDrawData;
		PrimitiveIndicesContainer m_primitiveIndicesContainer;

		// Scene Lights
		Vector<LightDrawData> m_lightDrawData;
		Vector<size_t> m_removedLightDataIndices;
		Vector<size_t> m_freeLightDataIndices;

		Vector<InvalidDrawData> m_invalidLightDataIndices;

		Map<UUID64, uint32_t> m_lightIndexFromLightID;

		GPUSceneBuffers m_buffers;

		EntityScene* m_scene = nullptr;

		uint32_t m_currentIndividualMeshCount = 0;
		uint32_t m_currentBoneCount = 0;
		uint32_t m_currentMeshletCount = 0;
	};
}
