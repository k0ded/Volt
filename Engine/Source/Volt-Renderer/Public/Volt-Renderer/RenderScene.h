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
	class Material;
	class RenderGraph;
	class MotionWeaver;
	class RayTracingScene;

	struct SceneLightDescription;

	struct GPUSceneBuffers
	{
		Ref<GrowingGPUBuffer> meshesBuffer;
		Ref<GrowingGPUBuffer> sdfMeshesBuffer;
		Ref<GrowingGPUBuffer> materialsBuffer;
		Ref<GrowingGPUBuffer> primitiveDrawDataBuffer;
		Ref<GrowingGPUBuffer> prevPrimitiveDrawDataBuffer;
		Ref<GrowingGPUBuffer> sdfPrimitiveDrawDataBuffer;
		Ref<GrowingGPUBuffer> bonesBuffer;
		Ref<GrowingGPUBuffer> lightsBuffer;

		Ref<GrowingGPUBuffer> validPrimitiveDrawDatasBuffer;
	};

	class VTR_API RenderScene
	{
	public:
		RenderScene(EntityScene* sceneRef);
		~RenderScene();

		void Update(RenderGraph& renderGraph);
		void EndFrame(RenderGraph& renderGraph);

		void InvalidatePrimitiveInstance(UUID64 renderObject);

		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<Material> material, uint32_t subMeshIndex);
		UUID64 AddPrimitiveInstance(EntityID entityId, Ref<MotionWeaver> motionWeaver, Ref<Mesh> mesh, Ref<Material> material, uint32_t subMeshIndex);
		void RemovePrimitiveInstance(UUID64 id);

		void InvalidateLightInstance(UUID64 id);

		UUID64 AddLightInstance(EntityID entityId, const SceneLightDescription& description);
		void RemoveLightInstance(UUID64 id);

		VT_INLINE VT_NODISCARD const uint32_t GetRenderObjectCount() const { return static_cast<uint32_t>(m_renderPrimitives.size()); }
		VT_INLINE VT_NODISCARD const uint32_t GetIndividualMeshCount() const { return m_currentIndividualMeshCount; }
		VT_INLINE VT_NODISCARD const uint32_t GetIndividualMaterialCount() const { return static_cast<uint32_t>(m_individualMaterials.size()); }
		VT_INLINE VT_NODISCARD const uint32_t GetMeshletCount() const { return m_currentMeshletCount; }
		VT_INLINE VT_NODISCARD const uint32_t GetDrawCount() const { return m_renderPrimitives.empty() ? 0u : static_cast<uint32_t>(m_primitiveDrawData.size()); }
		VT_INLINE VT_NODISCARD const uint32_t GetLightCount() const { return static_cast<uint32_t>(m_renderLights.size()); }
		VT_INLINE VT_NODISCARD const uint32_t GetSDFPrimitiveCount() const { return static_cast<uint32_t>(m_sdfPrimitiveDrawData.size()); }

		VT_NODISCARD Weak<Material> GetMaterialFromID(const uint32_t materialId) const;

		VT_NODISCARD const uint32_t GetMeshID(Weak<Mesh> mesh, uint32_t subMeshIndex) const;
		VT_NODISCARD const uint32_t GetMaterialIndex(Weak<Material> material) const;
		VT_NODISCARD const uint32_t GetMeshIndex(Weak<Mesh> mesh) const;
		VT_NODISCARD const uint32_t GetPrimitiveIndexFromID(UUID64 primitiveId) const;

		VT_INLINE VT_NODISCARD const GPUSceneBuffers GetGPUSceneBuffers() const { return m_buffers; }

		VT_NODISCARD Vector<RenderPrimitiveData>::iterator begin() { return m_renderPrimitives.begin(); }
		VT_NODISCARD Vector<RenderPrimitiveData>::iterator end() { return m_renderPrimitives.end(); }

		VT_NODISCARD VT_INLINE const Vector<RenderLightData>& GetRenderLightData() const { return m_renderLights; }

		VT_NODISCARD const Vector<RenderPrimitiveData>::const_iterator cbegin() const { return m_renderPrimitives.cbegin(); }
		VT_NODISCARD const Vector<RenderPrimitiveData>::const_iterator cend() const { return m_renderPrimitives.cend(); }

		VT_NODISCARD const RenderPrimitiveData& GetPrimitiveDataFromID(UUID64 id) const;
		VT_NODISCARD const RenderLightData& GetLightDataFromID(UUID64 id) const;

		VT_NODISCARD VT_INLINE std::span<const GPUMesh> GetGPUMeshes() const { return m_gpuMeshes; }
		VT_NODISCARD VT_INLINE std::span<const PrimitiveDrawData> GetPrimitiveDrawData() const { return m_primitiveDrawData; }
		VT_NODISCARD VT_INLINE Ref<RayTracingScene> GetRayTracingScene() const { return m_rayTracingScene; }

	private:
		void BuildGPUMaterial(Weak<Material> material, GPUMaterial& gpuMaterial);

		void BuildSinglePrimitiveDrawData(PrimitiveDrawData& primitiveDrawData, const RenderPrimitiveData& renderPrimitive);
		void BuildSingleSDFPrimitiveDrawData(SDFPrimitiveDrawData& primtiveDrawData, const RenderPrimitiveData& renderPrimitive);
		void BuildSingleLightDrawData(LightDrawData& lightDrawData, RenderLightData& renderLight);

		void TryAddMesh(Ref<Mesh> mesh);
		void TryAddMaterial(Ref<Material> material);

		void UpdateInvalidMaterials(RenderGraph& renderGraph);
		void UpdateInvalidMeshes(RenderGraph& renderGraph);
		void UpdateInvalidPrimitiveData(RenderGraph& renderGraph);
		void CompactValidPrimitiveDrawDatas(RenderGraph& renderGraph);

		void UpdateInvalidLights(RenderGraph& renderGraph);

		VT_NODISCARD RenderLightData& GetLightDataFromID(UUID64 id);

		struct InvalidMaterial
		{
			Weak<Material> material;
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

		Ref<RayTracingScene> m_rayTracingScene;

		Vector<UUID64> m_animatedRenderObjects;
		Vector<RenderPrimitiveData> m_renderPrimitives;
		Vector<RenderLightData> m_renderLights;

		Vector<GPUMesh> m_gpuMeshes;
		Vector<GPUMeshSDF> m_gpuSDFMeshes;
		Vector<GPUMaterial> m_gpuMaterials;

		Vector<InvalidMaterial> m_invalidMaterials;
		Vector<InvalidMesh> m_invalidMeshes;

		vt::map<AssetHandle, size_t> m_materialIndexFromAssetHandle;
		vt::map<size_t, size_t> m_gpuMeshIndexFromMeshAssetHash;
		vt::map<size_t, uint32_t> m_meshSubMeshToGPUMeshIndex;
		vt::map<size_t, uint32_t> m_meshSubMeshToGPUMeshSDFIndex;

		Vector<Weak<Mesh>> m_individualMeshes;
		Vector<Weak<Material>> m_individualMaterials;
		Vector<glm::mat4> m_animationBufferStorage;

		// Scene Primitives
		Vector<PrimitiveDrawData> m_primitiveDrawData;
		Vector<SDFPrimitiveDrawData> m_sdfPrimitiveDrawData;

		Vector<InvalidDrawData> m_invalidPrimitiveDataIndices;
		Vector<InvalidDrawData> m_invalidSDFPrimitiveDataIndices;

		Vector<size_t> m_removedPrimitiveDataIndices;
		Vector<size_t> m_freePrimitiveDataIndices;

		vt::map<UUID64, uint32_t> m_primitiveIndexFromPrimitiveID;
		vt::map<UUID64, uint32_t> m_sdfPrimitiveIndexFromPrimitiveID;

		// Scene Lights
		Vector<LightDrawData> m_lightDrawData;
		Vector<size_t> m_removedLightDataIndices;
		Vector<size_t> m_freeLightDataIndices;

		Vector<InvalidDrawData> m_invalidLightDataIndices;

		vt::map<UUID64, uint32_t> m_lightIndexFromLightID;

		GPUSceneBuffers m_buffers;

		EntityScene* m_scene = nullptr;

		uint32_t m_currentIndividualMeshCount = 0;
		uint32_t m_currentBoneCount = 0;
		uint32_t m_currentMeshletCount = 0;

		UUID64 m_materialChangedCallbackID;
		UUID64 m_meshChangedCallbackID;
	};
}
