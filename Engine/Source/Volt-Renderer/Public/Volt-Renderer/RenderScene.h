#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include "Volt-Renderer/MeshPassProcessor.h"
#include "Volt-Renderer/RenderScene/RenderSceneUpdateQueue.h"
#include "Volt-Renderer/RenderPrimitiveDataContainer.h"
#include "Volt-Renderer/Debug/DebugRenderer.h"

#include <RenderCore/Resources/GrowingGPUBuffer.h>
#include <RHIModule/RayTracing/RayTracingResuorceTable.h>
#include <CoreUtilities/Containers/Map.h>

#include <EventSystem/EventListener.h>
#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

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
	class TempAnimator;

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

	class VTR_API RenderScene : public EventListener
	{
	public:
		DECLARE_MULTICAST_DELEGATE_OneParam(RenderPrimitiveAddedDelegate, const RenderPrimitiveData*);
		DECLARE_MULTICAST_DELEGATE_OneParam(RenderPrimitiveRemovedDelegate, const RenderPrimitiveData*);

		RenderScene(EntityScene* sceneRef);
		~RenderScene();

		void Update(RenderGraph& renderGraph);
		void EndFrame(RenderGraph& renderGraph);
		void RenderDebug(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef dstDepth);

		RenderPrimitiveID AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		RenderPrimitiveID AddPrimitiveInstance(EntityID entityId, Ref<TempAnimator> animator, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex);
		void RemovePrimitiveInstance(RenderPrimitiveID id);
		void InvalidatePrimitiveInstance(RenderPrimitiveID renderObject);

		RenderPrimitiveID AddLightInstance(EntityID entityId, const SceneLightDescription& description);
		void RemoveLightInstance(RenderPrimitiveID id);
		void InvalidateLightInstance(RenderPrimitiveID id);

		RayTracingInstanceID AddRayTracingInstance(EntityID entityId, Ref<Mesh> mesh, RenderPrimitiveID primitiveId);
		void RemoveRayTracingInstance(RayTracingInstanceID instanceId);
		void InvalidateRayTracingInstance(RayTracingInstanceID instanceId);

		void OnRenderPrimitiveAdded(const RenderPrimitiveData* renderPrimitive);
		void OnRenderPrimitiveRemoved(const RenderPrimitiveData* renderPrimitive);
		Vector<RenderPrimitiveData*> GetRenderPrimitives() const;

		VT_INLINE VT_NODISCARD uint32_t GetLightCount() const { return static_cast<uint32_t>(m_renderLights.size()); }

		VT_NODISCARD uint32_t GetMaterialIndex(Weak<RenderMaterial> material) const;
		VT_NODISCARD uint32_t GetPrimitiveIndexFromID(RenderPrimitiveID primitiveId) const;
		VT_NODISCARD VT_INLINE uint32_t GetMaxPrimitiveIndex() const { return static_cast<uint32_t>(m_primitiveIndicesContainer.GetMaxIndex()); }

		VT_INLINE VT_NODISCARD const GPUSceneBuffers GetGPUSceneBuffers() const { return m_buffers; }
		VT_NODISCARD GPUSceneParameters GetGPUSceneParameters(RenderGraph& renderGraph) const;

		VT_NODISCARD VT_INLINE const Vector<RenderLightData>& GetRenderLightData() const { return m_renderLights; }

		VT_NODISCARD const RenderPrimitiveData* GetPrimitiveDataFromID(RenderPrimitiveID id) const;
		VT_NODISCARD const RenderLightData& GetLightDataFromID(RenderPrimitiveID id) const;

		VT_NODISCARD VT_INLINE std::span<const GPUMesh> GetGPUMeshes() const { return m_gpuMeshes; }
		VT_NODISCARD VT_INLINE std::span<const PrimitiveDrawData> GetPrimitiveDrawData() const { return m_primitiveDrawData; }
		VT_NODISCARD VT_INLINE Ref<RayTracingScene> GetRayTracingScene() const { return m_rayTracingScene; }
		VT_NODISCARD VT_INLINE RefPtr<RHI::RayTracingResourceTable> GetRayTracingResourceTable() const { return m_rayTracingResourceTable; }

		VT_NODISCARD VT_INLINE RenderPrimitiveAddedDelegate& GetRenderPrimitiveAddedDelegate() { return m_renderPrimitiveAddedDelegate; }
		VT_NODISCARD VT_INLINE RenderPrimitiveRemovedDelegate& GetRenderPrimitiveRemovedDelegate() { return m_renderPrimitiveRemovedDelegate; }

	private:
		void BuildGPUMaterial(Weak<RenderMaterial> material, GPUMaterial& gpuMaterial);
		void BuildGPUMesh(Weak<Mesh> mesh, uint32_t subMeshIndex, GPUMesh& outGPUMesh);

		void BuildSinglePrimitiveDrawData(PrimitiveDrawData& primitiveDrawData, const RenderPrimitiveData& renderPrimitive);
		void BuildSingleLightDrawData(LightDrawData& lightDrawData, RenderLightData& renderLight);

		void TryAddMesh(Ref<Mesh> mesh);
		void TryAddMaterial(Ref<RenderMaterial> material);

		void UpdateInvalidMaterials(RenderGraph& renderGraph);
		void UpdateInvalidMeshes(RenderGraph& renderGraph);
		void UpdateInvalidPrimitiveData(RenderGraph& renderGraph);
		void CompactValidPrimitiveDrawDatas(RenderGraph& renderGraph);

		void UpdateInvalidLights(RenderGraph& renderGraph);

		void VisualizeRenderPrimitives();

		bool OnPreRenderEvent(AppPreRenderEvent& event);

		VT_NODISCARD RenderLightData& GetLightDataFromID(UUID64 id);
		VT_NODISCARD PrimitiveDrawData& GetPrimitiveDrawDataFromIndex(size_t index);

		void ProcessQueuedUpdateOperations();
		void ProcessAddPrimitiveInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate);
		void ProcessAddLightInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate);
		void ProcessAddRayTracingInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate);
		void ProcessQueuedRemove(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate);
		void ProcessQueuedInvalidation(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate);

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
			VT_INLINE bool Contains(UUID64 id) const { return m_primitiveIndexFromPrimitiveID.contains(id); }

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

		template<typename Func>
		struct Callback
		{
			Func callback;
			UUID32 id;
		};

		Ref<RayTracingScene> m_rayTracingScene;

		Vector<UUID64> m_animatedRenderObjects;
		Vector<RenderLightData> m_renderLights;

		Vector<GPUMesh> m_gpuMeshes;
		Vector<GPUMaterial> m_gpuMaterials;

		Vector<InvalidMaterial> m_invalidMaterials;
		Vector<InvalidMesh> m_invalidMeshes;

		Map<size_t, size_t> m_gpuMaterialIndexFromMaterialHash;
		Map<size_t, uint32_t> m_meshSubMeshToGPUMeshIndex;
		Map<uint32_t, MeshAndSubMeshIndex> m_gpuMeshIndexToMeshAndSubMeshIndex;

		Vector<Ref<Mesh>> m_individualMeshes;
		Vector<Ref<RenderMaterial>> m_individualMaterials;
		Vector<glm::mat4> m_animationBufferStorage;

		RefPtr<RHI::RayTracingResourceTable> m_rayTracingResourceTable;

		// Render primitives
		RenderPrimitiveAddedDelegate m_renderPrimitiveAddedDelegate;
		RenderPrimitiveRemovedDelegate m_renderPrimitiveRemovedDelegate;
		RenderPrimitiveDataContainer m_renderPrimitiveDataContainer;

		RenderSceneUpdateQueue m_updateQueue;

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

		uint32_t m_currentBoneCount = 0;
		uint32_t m_frameIndex = 0;
		
		// Debug
		DebugRenderer m_debugRenderer;
	};
}
