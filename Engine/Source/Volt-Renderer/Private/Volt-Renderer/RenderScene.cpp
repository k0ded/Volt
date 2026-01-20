#include "vrpch.h"
#include "Volt-Renderer/RenderScene.h"

#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"
#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/Debug/DebugRenderer.h"

#include <RenderCore/Shader/GlobalShader.h>

#include <Volt-Animation/TempAnimator.h>
#include <Volt-Animation/Assets/Skeleton.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>
#include <Volt-Core/Algorithms.h>

#include <EntitySystem/EntityScene.h>
#include <EntitySystem/Entity.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/RHIFeatures.h>

#include <CoreUtilities/FormatterExtension.h>

#include <ranges>

VT_DEFINE_LOG_CATEGORY(LogRenderScene);

namespace Volt
{
	static ConsoleVariable<int32_t> s_logRenderSceneUpdatedCVar(
		"r.RenderScene.LogUpdates", 
		0,
		"Whether or not to log Render Scene updates"
	);

	static ConsoleVariable<int32_t> s_visualizeRenderScenePrimitiveBoundingSpheres(
		"r.RenderScene.VisualizePrimitiveBoundingSpheres",
		0,
		"Whether of not to visualize render primitive bounding spheres."
	);

	RenderScene::RenderScene(EntityScene* sceneRef)
		: m_scene(sceneRef)
	{
		m_buffers.meshesBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(GPUMesh), "GPU Meshes");
		m_buffers.materialsBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(GPUMaterial), "GPU Materials");
		m_buffers.primitiveDrawDataBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(PrimitiveDrawData), "Primitive Draw Data");
		m_buffers.prevPrimitiveDrawDataBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(PrimitiveDrawData), "Prev Primitive Draw Data");
		m_buffers.bonesBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(glm::mat4), "GPU Bones");
		m_buffers.lightsBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(LightDrawData), "Lights");
		m_buffers.validPrimitiveDrawDatasBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(uint32_t), "Compacted Valid Primitive Draw Datas", RHI::BufferUsage::TexelBuffer);

		// Setup invalid mesh
		{
			GPUMesh& mesh = m_gpuMeshes.emplace_back();
			memset(&mesh, 0, sizeof(GPUMesh));
		}

		if (RHI::RHICanUseRayTracing())
		{
			m_rayTracingScene = CreateRef<RayTracingScene>(m_scene);
			m_rayTracingResourceTable = RHI::RayTracingResourceTable::Create();
		}

		RegisterListener<AppPreRenderEvent>(VT_BIND_EVENT_FN(RenderScene::OnPreRenderEvent)); 
	}

	RenderScene::~RenderScene()
	{
	}

	void RenderScene::Update(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		renderGraph.BeginMarker("RenderScene::Update");

		// Temporary animation sampling
		m_currentBoneCount = 0;
		for (const auto& animatedObject : m_animatedRenderObjects)
		{
			auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndicesContainer.GetIndexFromID(animatedObject));
			primitiveDrawData.boneOffset = m_currentBoneCount;

			const auto& renderObject = GetPrimitiveDataFromID(animatedObject);
			AssetReference<Skeleton> skeleton = renderObject->animator->GetSkeleton();

			if (skeleton.IsValid())
			{
				m_currentBoneCount += static_cast<uint32_t>(skeleton->GetJointCount());
				// Mark primitive as invalid.
				InvalidatePrimitiveInstance(animatedObject);
			}
		}

		m_animationBufferStorage.resize(m_currentBoneCount);
		if (m_currentBoneCount > 0)
		{
			for (const auto& animatedObject : m_animatedRenderObjects)
			{
				const auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndicesContainer.GetIndexFromID(animatedObject));
				const auto& renderObject = GetPrimitiveDataFromID(animatedObject);

				const auto sample = renderObject->animator->Sample();
				memcpy_s(m_animationBufferStorage.data() + primitiveDrawData.boneOffset, sizeof(glm::mat4) * sample.size(), sample.data(), sizeof(glm::mat4) * sample.size());
			}
		}

		ProcessQueuedUpdateOperations();

		UpdateInvalidMaterials(renderGraph);
		UpdateInvalidMeshes(renderGraph);
		UpdateInvalidLights(renderGraph);
		UpdateInvalidPrimitiveData(renderGraph);
		CompactValidPrimitiveDrawDatas(renderGraph);


		if (m_currentBoneCount > 0)
		{
			auto bonesBuffer = m_buffers.bonesBuffer;

			bonesBuffer->GrowIfRequired(m_animationBufferStorage.size());

			bonesBuffer->GetResource()->SetData(m_animationBufferStorage.data(), m_animationBufferStorage.size() * sizeof(glm::mat4));
			m_animationBufferStorage.clear();
		}

		if (m_rayTracingScene)
		{
			m_rayTracingScene->Update();
		}

		if (s_visualizeRenderScenePrimitiveBoundingSpheres.GetValue())
		{
			VisualizeRenderPrimitives();
		}

		renderGraph.EndMarker();
	}

	void RenderScene::EndFrame(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		m_buffers.prevPrimitiveDrawDataBuffer->GrowIfRequired(m_buffers.primitiveDrawDataBuffer->GetResource()->GetCount());

		RGBufferRef srcPrimitiveData = renderGraph.RegisterExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource());
		RGBufferRef dstPrimitiveData = renderGraph.RegisterExternalBuffer(m_buffers.prevPrimitiveDrawDataBuffer->GetResource());

		AddCopyBufferPass(renderGraph, srcPrimitiveData, 0, dstPrimitiveData, 0, m_buffers.primitiveDrawDataBuffer->GetResource()->GetByteSize(), "Copy PrimitiveDrawData");
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		return m_updateQueue.AddPrimitiveInstance(entityId, nullptr, mesh, material, subMeshIndex);
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<TempAnimator> animator, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		return m_updateQueue.AddPrimitiveInstance(entityId, animator, mesh, material, subMeshIndex);
	}

	void RenderScene::RemovePrimitiveInstance(RenderPrimitiveID id)
	{
		m_updateQueue.RemovePrimitiveInstance(id);
	}

	void RenderScene::InvalidatePrimitiveInstance(RenderPrimitiveID renderObject)
	{
		m_updateQueue.InvalidatePrimitiveInstance(renderObject);
	}

	UUID64 RenderScene::AddLightInstance(EntityID entityId, const SceneLightDescription& description)
	{
		return m_updateQueue.AddLightInstance(entityId, description);
	}

	void RenderScene::RemoveLightInstance(RenderPrimitiveID id)
	{
		return m_updateQueue.RemoveLightInstance(id);
	}

	RayTracingInstanceID RenderScene::AddRayTracingInstance(EntityID entityId, Ref<Mesh> mesh, RenderPrimitiveID primitiveId)
	{
		return m_updateQueue.AddRayTracingInstance(entityId, mesh, primitiveId);
	}

	void RenderScene::RemoveRayTracingInstance(RayTracingInstanceID instanceId)
	{
		m_updateQueue.RemoveRayTracingInstance(instanceId);
	}

	void RenderScene::InvalidateRayTracingInstance(RayTracingInstanceID instanceId)
	{
		m_updateQueue.InvalidateRayTracingInstance(instanceId);
	}

	void RenderScene::InvalidateLightInstance(UUID64 id)
	{
		m_updateQueue.InvalidateLightInstance(id);
	}

	void RenderScene::OnRenderPrimitiveAdded(const RenderPrimitiveData* renderPrimitive)
	{
		m_renderPrimitiveAddedDelegate.Broadcast(renderPrimitive);
	}

	void RenderScene::OnRenderPrimitiveRemoved(const RenderPrimitiveData* renderPrimitive)
	{
		m_renderPrimitiveRemovedDelegate.Broadcast(renderPrimitive);
	}

	Vector<RenderPrimitiveData*> RenderScene::GetRenderPrimitives() const
	{
		return m_renderPrimitiveDataContainer.GetRenderPrimitives();
	}

	uint32_t RenderScene::GetMaterialIndex(Weak<RenderMaterial> material) const
	{
		auto it = std::find_if(m_individualMaterials.begin(), m_individualMaterials.end(), [&](Weak<RenderMaterial> lhs)
		{
			return lhs.Get() == material.Get();
		});

		if (it != m_individualMaterials.end())
		{
			return static_cast<uint32_t>(std::distance(m_individualMaterials.begin(), it));
		}

		return std::numeric_limits<uint32_t>::max();
	}

	uint32_t RenderScene::GetPrimitiveIndexFromID(RenderPrimitiveID primitiveId) const
	{
		return static_cast<uint32_t>(m_primitiveIndicesContainer.GetIndexFromID(primitiveId));
	}

	GPUSceneParameters RenderScene::GetGPUSceneParameters(RenderGraph& renderGraph) const
	{
		GPUSceneParameters result;
		result.PrimitiveDrawDataBuffer = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource()));
		result.PrevPrimitiveDrawDataBuffer = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.prevPrimitiveDrawDataBuffer->GetResource()));
		result.GPUMeshes = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.meshesBuffer->GetResource()));
		result.SceneLights = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.lightsBuffer->GetResource()));
		result.AnimatedBones = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.bonesBuffer->GetResource()));

		return result;
	}

	const RenderPrimitiveData* RenderScene::GetPrimitiveDataFromID(RenderPrimitiveID id) const
	{
		return m_renderPrimitiveDataContainer.GetFromID(id);
	}

	void RenderScene::VisualizeRenderPrimitives()
	{
		auto transformPosition = [](const glm::vec3& pos, const glm::vec3& translation, const glm::vec3& scale, const glm::quat& rotation) 
		{
			glm::vec3 v = pos * scale;
			glm::vec3 rotXYZ = glm::vec3(rotation.x, rotation.y, rotation.z);

			v = v + 2.f * glm::cross(rotXYZ, glm::cross(rotXYZ, v) + rotation.w * v);
			v += translation;

			return v;
		};

		for (const PrimitiveDrawData& primitive : m_primitiveDrawData)
		{
			if (EnumValueContainsAnyFlag(primitive.flags, PrimitiveFlags::Valid))
			{
				const GPUMesh& gpuMesh = m_gpuMeshes.at(primitive.meshId);

				const float maxScale = glm::max(glm::max(primitive.transform.scale.x, primitive.transform.scale.y), primitive.transform.scale.z);
				const glm::vec3 center = transformPosition(gpuMesh.center, primitive.transform.position, primitive.transform.scale, primitive.transform.rotation);

				Renderer::GetDebugRenderer().DrawLineSphere(center, maxScale * gpuMesh.radius, 1.f);
			}
		}
	}

	bool RenderScene::OnPreRenderEvent(AppPreRenderEvent& event)
	{
		if (RHI::RHICanUseRayTracing())
		{
			m_rayTracingResourceTable->Update(static_cast<uint32_t>(event.GetFrameIndex()));
		}

		return false;
	}

	VT_NODISCARD const RenderLightData& RenderScene::GetLightDataFromID(RenderPrimitiveID id) const
	{
		auto it = std::ranges::find_if(m_renderLights, [id](const auto& light)
		{
			return light.id == id;
		});

		if (it == m_renderLights.end())
		{
			static RenderLightData nullObject;
			return nullObject;
		}

		return *it;
	}

	void RenderScene::ProcessQueuedUpdateOperations()
	{
		VT_PROFILE_FUNCTION();

		RenderSceneUpdateQueue::QueuedUpdate queuedUpdate;
		while (m_updateQueue.TryPop(queuedUpdate))
		{
			if (queuedUpdate.operation == RenderSceneUpdateQueue::UpdateOperation::Add)
			{
				if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Primitive)
				{
					ProcessAddPrimitiveInstance(queuedUpdate);
				}
				else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Light)
				{
					ProcessAddLightInstance(queuedUpdate);
				}
				else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::RayTracingInstance)
				{
					ProcessAddRayTracingInstance(queuedUpdate);
				}
			}
			else if (queuedUpdate.operation == RenderSceneUpdateQueue::UpdateOperation::Remove)
			{
				ProcessQueuedRemove(queuedUpdate);
			}
			else if (queuedUpdate.operation == RenderSceneUpdateQueue::UpdateOperation::Invalidate)
			{
				ProcessQueuedInvalidation(queuedUpdate);
			}
		}
	}

	void RenderScene::ProcessAddPrimitiveInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate)
	{
		UUID64 newId = queuedUpdate.primitiveInfo.id;
		RenderPrimitiveData* newRenderPrimitive = m_renderPrimitiveDataContainer.Create(newId);

		if (queuedUpdate.primitiveInfo.animator)
		{
			m_animatedRenderObjects.emplace_back(newId);
		}

		newRenderPrimitive->entityId = queuedUpdate.primitiveInfo.entityId;
		newRenderPrimitive->mesh = queuedUpdate.primitiveInfo.mesh;
		newRenderPrimitive->material = queuedUpdate.primitiveInfo.material;
		newRenderPrimitive->subMeshIndex = queuedUpdate.primitiveInfo.subMeshIndex;
		newRenderPrimitive->animator = queuedUpdate.primitiveInfo.animator;

		TryAddMaterial(queuedUpdate.primitiveInfo.material);
		TryAddMesh(queuedUpdate.primitiveInfo.mesh);

		size_t primitiveDrawDataIndex = m_primitiveIndicesContainer.GetAvailableIndex(newId);

		newRenderPrimitive->primitiveIndex = static_cast<uint32_t>(primitiveDrawDataIndex);

		PrimitiveDrawData& primitiveDrawData = GetPrimitiveDrawDataFromIndex(primitiveDrawDataIndex);

		BuildSinglePrimitiveDrawData(primitiveDrawData, *newRenderPrimitive);
		InvalidatePrimitiveInstance(newId);

		OnRenderPrimitiveAdded(newRenderPrimitive);
	}

	void RenderScene::ProcessAddLightInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate)
	{
		UUID64 id = queuedUpdate.lightInfo.id;

		auto& lightInstance = m_renderLights.emplace_back();
		lightInstance.id = id;
		lightInstance.entityId = queuedUpdate.lightInfo.entityId;
		lightInstance.description = queuedUpdate.lightInfo.lightDescription;

		constexpr size_t SizeMax = std::numeric_limits<size_t>::max();

		size_t lightDrawDataIndex = SizeMax;

		if (!m_freeLightDataIndices.empty())
		{
			lightDrawDataIndex = m_freeLightDataIndices.back();
			m_freeLightDataIndices.pop_back();
		}

		LightDrawData& lightDrawData = (lightDrawDataIndex != SizeMax) ? m_lightDrawData.at(lightDrawDataIndex) : m_lightDrawData.emplace_back();
		lightDrawData.lightType = queuedUpdate.lightInfo.lightDescription.lightType;

		m_lightIndexFromLightID[id] = static_cast<uint32_t>((lightDrawDataIndex != SizeMax) ? lightDrawDataIndex : m_lightDrawData.size() - 1);
		InvalidateLightInstance(id);
	}

	void RenderScene::ProcessAddRayTracingInstance(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate)
	{
		m_rayTracingScene->AddInstanceWithID(
			queuedUpdate.rayTracingInstanceInfo.mesh,
			queuedUpdate.rayTracingInstanceInfo.entityId,
			GetPrimitiveIndexFromID(queuedUpdate.rayTracingInstanceInfo.renderScenePrimitiveId),
			queuedUpdate.rayTracingInstanceInfo.id
		);
	}

	void RenderScene::ProcessQueuedRemove(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate)
	{
		if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Primitive)
		{
			m_primitiveIndicesContainer.FreeIndexWithID(queuedUpdate.id);

			RenderPrimitiveData* renderPrimitiveData = m_renderPrimitiveDataContainer.GetFromID(queuedUpdate.id);
			const bool isAnimated = renderPrimitiveData->IsAnimated();

			OnRenderPrimitiveRemoved(renderPrimitiveData);

			m_renderPrimitiveDataContainer.Destroy(renderPrimitiveData);

			if (isAnimated)
			{
				auto animIt = std::find_if(m_animatedRenderObjects.begin(), m_animatedRenderObjects.end(), [id = queuedUpdate.id](const auto& obj)
				{
					return obj == id;
				});

				if (animIt != m_animatedRenderObjects.end())
				{
					m_animatedRenderObjects.erase(animIt);
				}
			}
		}
		else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Light)
		{
			VT_ENSURE(m_lightIndexFromLightID.contains(queuedUpdate.id));

			const size_t lightDataIndex = m_lightIndexFromLightID.at(queuedUpdate.id);

			m_lightDrawData.at(lightDataIndex).flags = LightFlags::Invalid;
			m_removedLightDataIndices.emplace_back(lightDataIndex);
			m_freeLightDataIndices.emplace_back(lightDataIndex);

			m_lightIndexFromLightID.erase(queuedUpdate.id);

			auto it = std::ranges::find_if(m_renderLights, [id = queuedUpdate.id](const auto& obj)
			{
				return obj.id == id;
			});

			if (it != m_renderLights.end())
			{
				m_renderLights.erase(it);
			}
		}
		else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::RayTracingInstance)
		{
			m_rayTracingScene->RemoveInstance(queuedUpdate.id);
		}
	}

	void RenderScene::ProcessQueuedInvalidation(const RenderSceneUpdateQueue::QueuedUpdate& queuedUpdate)
	{
		if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Primitive)
		{
			m_primitiveIndicesContainer.InvalidateIndexWithID(queuedUpdate.id);
		}
		else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::Light)
		{
			if (m_lightIndexFromLightID.contains(queuedUpdate.id))
			{
				m_invalidLightDataIndices.emplace_back(queuedUpdate.id, m_lightIndexFromLightID.at(queuedUpdate.id));
			}
		}
		else if (queuedUpdate.type == RenderSceneUpdateQueue::UpdateType::RayTracingInstance)
		{
			m_rayTracingScene->InvalidateInstance(queuedUpdate.id);
		}
	}

	void RenderScene::BuildGPUMaterial(Weak<RenderMaterial> material, GPUMaterial& gpuMaterial)
	{
		gpuMaterial.textureCount = 0;
		
		// #TODO_Ivar: Bindless-support
#if 0
		for (const auto& texture : material->GetTextures())
		{
			gpuMaterial.textures[gpuMaterial.textureCount] = texture.GetResource();
			gpuMaterial.samplers[gpuMaterial.textureCount] = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::X16>()->GetResourceHandle();
			gpuMaterial.textureCount++;
		}
#endif
	}

	void RenderScene::BuildGPUMesh(Weak<Mesh> mesh, uint32_t subMeshIndex, GPUMesh& outGPUMesh)
	{
		outGPUMesh = mesh->GetGPUMeshes().at(subMeshIndex);

		if (RHI::RHICanUseRayTracing())
		{
			outGPUMesh.RT_vertexPositionsBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexPositionsBuffer());
			outGPUMesh.RT_vertexAnimationInfoBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexAnimationInfoBuffer());
			outGPUMesh.RT_vertexMaterialBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexMaterialBuffer());
			outGPUMesh.RT_indexBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetIndexBuffer());
		}
	}

	void RenderScene::BuildSinglePrimitiveDrawData(PrimitiveDrawData& primitiveDrawData, const RenderPrimitiveData& renderObject)
	{
		Entity entity = m_scene->GetEntityFromID(renderObject.entityId);
		if (!entity.IsValid())
		{
			return;
		}

		const size_t hash = Math::HashCombine(renderObject.mesh->GetHash(), std::hash<uint32_t>()(renderObject.subMeshIndex));
		const uint32_t meshId = m_meshSubMeshToGPUMeshIndex.contains(hash) ? m_meshSubMeshToGPUMeshIndex.at(hash) : 0;

		primitiveDrawData.transform.position = entity.GetPosition();
		primitiveDrawData.transform.scale = entity.GetScale();
		primitiveDrawData.transform.rotation = entity.GetRotation();
		primitiveDrawData.meshId = meshId;
		primitiveDrawData.entityId = entity.GetID();
		primitiveDrawData.materialId = GetMaterialIndex(renderObject.material);
		primitiveDrawData.meshletStartOffset = renderObject.meshletStartOffset;
		primitiveDrawData.isAnimated = renderObject.IsAnimated();
		primitiveDrawData.flags = PrimitiveFlags::Valid;
	}

	void RenderScene::BuildSingleLightDrawData(LightDrawData& lightDrawData, RenderLightData& renderLight)
	{
		Entity entity = m_scene->GetEntityFromID(renderLight.entityId);
		if (!entity.IsValid())
		{
			return;
		}

		lightDrawData.lightType = renderLight.description.lightType;
		lightDrawData.intensity = renderLight.description.intensity;
		lightDrawData.color = renderLight.description.color;
		lightDrawData.position = entity.GetPosition();

		if (renderLight.description.castShadows)
		{
			lightDrawData.flags |= LightFlags::CastShadows;
		}

		if (lightDrawData.lightType == SceneLightType::Point)
		{
			lightDrawData.lightSpecific.x = renderLight.description.radius;
			lightDrawData.lightSpecific.y = renderLight.description.falloff;
		}
		else if (lightDrawData.lightType == SceneLightType::Spot)
		{
			const float cosInnerAngle = glm::cos(glm::radians(renderLight.description.innerAngle));
			const float cosOuterAngle = glm::cos(glm::radians(renderLight.description.outerAngle));

			lightDrawData.direction = -entity.GetForward();
			lightDrawData.lightSpecific.x = renderLight.description.range;
			lightDrawData.lightSpecific.y = renderLight.description.falloff;
			lightDrawData.lightSpecific.z = 1.f / glm::max((cosInnerAngle - cosOuterAngle), 0.001f);
			lightDrawData.lightSpecific.w = -cosOuterAngle * lightDrawData.lightSpecific.z;
		}
		else if (lightDrawData.lightType == SceneLightType::Directional)
		{
			lightDrawData.direction = -entity.GetForward();
			lightDrawData.lightSpecific.x = glm::radians(renderLight.description.sunRadius);
		}
		else if (lightDrawData.lightType == SceneLightType::Sky)
		{
			lightDrawData.lightSpecific.x = renderLight.description.lod;
		}

		renderLight.description.direction = lightDrawData.direction;
	}

	void RenderScene::TryAddMesh(Ref<Mesh> mesh)
	{
		VT_PROFILE_FUNCTION();

		auto it = std::find(m_individualMeshes.begin(), m_individualMeshes.end(), mesh);
		if (it != m_individualMeshes.end())
		{
			return;
		}

		// Add vertex and index buffers to ray tracing table
		uint32_t RT_vertexPositionsBuffer = 0;
		uint32_t RT_vertexAnimationInfoBuffer = 0;
		uint32_t RT_vertexMaterialBuffer = 0;
		uint32_t RT_indexBuffer = 0;

		if (RHI::RHICanUseRayTracing())
		{
			m_rayTracingResourceTable->AddBuffer(mesh->GetVertexPositionsBuffer());
			m_rayTracingResourceTable->AddBuffer(mesh->GetVertexAnimationInfoBuffer());
			m_rayTracingResourceTable->AddBuffer(mesh->GetVertexMaterialBuffer());
			m_rayTracingResourceTable->AddBuffer(mesh->GetIndexBuffer());

			RT_vertexPositionsBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexPositionsBuffer());
			RT_vertexAnimationInfoBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexAnimationInfoBuffer());
			RT_vertexMaterialBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetVertexMaterialBuffer());
			RT_indexBuffer = m_rayTracingResourceTable->GetBufferSlotIndex(mesh->GetIndexBuffer());
		}

		const size_t newMeshIndex = m_individualMeshes.size();

		m_individualMeshes.emplace_back(mesh);

		size_t currentIndex = m_gpuMeshes.size();

		for (size_t i = 0; i < mesh->GetGPUMeshes().size(); ++i)
		{
			const uint32_t subMeshIndex = static_cast<uint32_t>(i);

			BuildGPUMesh(mesh, subMeshIndex, m_gpuMeshes.emplace_back());

			const size_t meshHash = Math::HashCombine(mesh->GetHash(), std::hash<uint32_t>()(subMeshIndex));
			m_meshSubMeshToGPUMeshIndex[meshHash] = static_cast<uint32_t>(currentIndex);
			m_gpuMeshIndexToMeshAndSubMeshIndex[static_cast<uint32_t>(currentIndex)] = { newMeshIndex, subMeshIndex };
			m_invalidMeshes.emplace_back(mesh, subMeshIndex, currentIndex);

			currentIndex++;
		}

		if (s_logRenderSceneUpdatedCVar.GetValue())
		{
			VT_LOGC(Trace, LogRenderScene, "Mesh {} was added to render scene!", mesh->GetName());
		}
	}

	void RenderScene::TryAddMaterial(Ref<RenderMaterial> material)
	{
		VT_PROFILE_FUNCTION();

		if (!material)
		{
			return;
		}

		auto it = std::find(m_individualMaterials.begin(), m_individualMaterials.end(), material);
		if (it != m_individualMaterials.end())
		{
			return;
		}

		const size_t gpuMaterialIndex = m_gpuMaterials.size();

		GPUMaterial& gpuMaterial = m_gpuMaterials.emplace_back();
		BuildGPUMaterial(material, gpuMaterial);

		m_individualMaterials.emplace_back(material);
		m_gpuMaterialIndexFromMaterialHash[material->GetHash()] = gpuMaterialIndex;
		m_invalidMaterials.emplace_back(material, gpuMaterialIndex);

		// Add textures to ray tracing table
		if (RHI::RHICanUseRayTracing())
		{
			for (const auto& [index, textureInfo] : material->GetTextures())
			{
				m_rayTracingResourceTable->AddTexture(textureInfo.texture.GetResource());
			}
		}
	
		if (s_logRenderSceneUpdatedCVar.GetValue())
		{
			VT_LOGC(Trace, LogRenderScene, "Material {} was added to render scene!", material->GetName());
		}
	}

	void RenderScene::UpdateInvalidMaterials(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		auto materialsBuffer = m_buffers.materialsBuffer;
		materialsBuffer->GrowIfRequired(m_individualMaterials.size());

		for (const auto& material : m_individualMaterials)
		{
			if (material->DoMaterialRequireUpdate())
			{
				m_invalidMaterials.emplace_back(material, m_gpuMaterialIndexFromMaterialHash.at(material->GetHash()));
				material->UpdateTextures();
				material->ClearStatus();
			}
		}

		if (!m_invalidMaterials.empty())
		{
			ScatteredBufferUpload<GPUMaterial> bufferUpload{ m_invalidMaterials.size() };

			for (const auto& invalidMaterial : m_invalidMaterials)
			{
				auto& data = bufferUpload.AddUploadItem(invalidMaterial.index);
				BuildGPUMaterial(invalidMaterial.material, data);

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Material {} was uploaded to index {}.", invalidMaterial.material->GetName(), invalidMaterial.index);
				}
			}

			bufferUpload.UploadTo(renderGraph, materialsBuffer->GetResource());
			m_invalidMaterials.clear();
		}
	}

	void RenderScene::UpdateInvalidMeshes(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		auto meshesBuffer = m_buffers.meshesBuffer;
		meshesBuffer->GrowIfRequired(m_gpuMeshes.size());

		for (const auto& mesh : m_individualMeshes)
		{
			if (mesh->DoMeshRequireUpdate())
			{
				for (uint32_t i = 0; i < mesh->GetNumSubMeshes(); ++i)
				{
					const size_t hash = Math::HashCombine(mesh->GetHash(), std::hash<uint32_t>()(i));
					m_invalidMeshes.emplace_back(mesh, i, m_meshSubMeshToGPUMeshIndex.at(hash));
				}

				mesh->ClearStatus();
			}
		}

		if (!m_invalidMeshes.empty())
		{
			ScatteredBufferUpload<GPUMesh> bufferUpload{ m_invalidMeshes.size() };

			for (const auto& invalidMesh : m_invalidMeshes)
			{
				auto& data = bufferUpload.AddUploadItem(invalidMesh.index);
				BuildGPUMesh(invalidMesh.mesh, invalidMesh.subMeshIndex, data);

				m_gpuMeshes[invalidMesh.index] = data;

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Mesh {} was uploaded to index {}.", invalidMesh.mesh->GetName(), invalidMesh.index);
				}
			}

			bufferUpload.UploadTo(renderGraph, meshesBuffer->GetResource());
			m_invalidMeshes.clear();
		}
	}

	void RenderScene::UpdateInvalidPrimitiveData(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		auto drawDataBuffer = m_buffers.primitiveDrawDataBuffer;
		drawDataBuffer->GrowIfRequired(m_primitiveDrawData.size());

		Vector<size_t> removedPrimitiveDataIndices = m_primitiveIndicesContainer.GetAndClearRemovedIndices();
		Vector<InvalidDrawData> invalidPrimitiveDataIndices = m_primitiveIndicesContainer.GetAndClearInvalidIndices();

		if (!invalidPrimitiveDataIndices.empty() || !removedPrimitiveDataIndices.empty())
		{
			ScatteredBufferUpload<PrimitiveDrawData> bufferUpload{ invalidPrimitiveDataIndices.size() + removedPrimitiveDataIndices.size() };

			Algo::ForEachParalellBlocking([&](uint32_t threadIdx, uint32_t elementIdx) 
			{
				const InvalidDrawData& invalidPrimitive = invalidPrimitiveDataIndices.at(elementIdx);

				const RenderPrimitiveData* renderObject = GetPrimitiveDataFromID(invalidPrimitive.id);
				PrimitiveDrawData& data = bufferUpload.AddUploadItem(invalidPrimitive.index);
				BuildSinglePrimitiveDrawData(data, *renderObject);

				// Copy the bone offset which is set before this is run.
				data.boneOffset = m_primitiveDrawData[invalidPrimitive.index].boneOffset;
				m_primitiveDrawData[invalidPrimitive.index] = data;

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					std::string logMessage = std::format(
						"Primitive Data with ID {} added: \n"
						"	- Entity: {}\n"
						"	- Index: {}\n"
						"	- Position: {}\n"
						"	- Rotation: {}\n"
						"	- Scale: {}\n",
						invalidPrimitive.id,
						data.entityId,
						invalidPrimitive.index,
						data.transform.position,
						data.transform.rotation,
						data.transform.scale
					);

					VT_LOGC_UNFORMATTED(Trace, LogRenderScene, logMessage);
				}
			
			}, static_cast<uint32_t>(invalidPrimitiveDataIndices.size()));

			for (const auto& removedPrimitiveIndex : removedPrimitiveDataIndices)
			{
				auto& data = bufferUpload.AddUploadItem(removedPrimitiveIndex);
				data.flags = PrimitiveFlags::Invalid;

				m_primitiveDrawData[removedPrimitiveIndex] = data;

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Primitive Data with index {} was removed.", removedPrimitiveIndex);
				}
			}

			bufferUpload.UploadTo(renderGraph, drawDataBuffer->GetResource());
		}
	}

	struct CompactValidDrawCallCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CompactValidDrawCallCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWValidPrimitiveDrawData)
			SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<PrimitiveDrawData>, PrimitiveDrawDataBuffer)
			SHADER_PARAMETER(uint32_t, PrimitiveDrawDataCount)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(CompactValidDrawCallCS, "Engine/Shaders/Source/RenderScene/CompactValidDrawCalls.hlsl", "MainCS", Compute);

	void RenderScene::CompactValidPrimitiveDrawDatas(RenderGraph& renderGraph)
	{
		// We need to make sure that the buffer is one larger than the count, because
		// the first index is used for the count.
		auto validPrimitiveDrawDataBuffer = m_buffers.validPrimitiveDrawDatasBuffer;

		const uint32_t primitiveDrawDataCount = m_buffers.primitiveDrawDataBuffer->GetResource()->GetCount();
		validPrimitiveDrawDataBuffer->GrowIfRequired(primitiveDrawDataCount + 1);

		RGBufferRef validPrimitiveDrawData = renderGraph.RegisterExternalBuffer(validPrimitiveDrawDataBuffer->GetResource());
		RGBufferRef primitiveDrawData = renderGraph.RegisterExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource());

		AddClearUAVPass(renderGraph, renderGraph.CreateUAV(validPrimitiveDrawData, RHI::PixelFormat::R32_UINT), 0u);

		CompactValidDrawCallCS::Parameters* passParameters = renderGraph.AllocParameters<CompactValidDrawCallCS::Parameters>();
		passParameters->RWValidPrimitiveDrawData = renderGraph.CreateUAV(validPrimitiveDrawData, RHI::PixelFormat::R32_UINT);
		passParameters->PrimitiveDrawDataBuffer = renderGraph.CreateSRV(primitiveDrawData);
		passParameters->PrimitiveDrawDataCount = primitiveDrawDataCount;

		constexpr uint32_t workGroupCount = 64;

		auto shader = ShaderMap::Get<CompactValidDrawCallCS>();
		ComputeShaderUtils::AddPass<CompactValidDrawCallCS>(renderGraph,
			"Compact Valid Primitive Draw Datas",
			shader,
			passParameters,
			{ Math::DivideRoundUp(primitiveDrawDataCount, workGroupCount), 1, 1 });
	}

	void RenderScene::UpdateInvalidLights(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		auto lightsBuffer = m_buffers.lightsBuffer;
		lightsBuffer->GrowIfRequired(m_lightDrawData.size());

		for (int32_t i = static_cast<int32_t>(m_removedLightDataIndices.size()) - 1; i >= 0; i--)
		{
			const size_t index = m_removedLightDataIndices.at(i);

			auto it = std::ranges::find_if(m_invalidLightDataIndices, [index](const InvalidDrawData& i) { return i.index == index; });
			if (it != m_invalidLightDataIndices.end())
			{
				m_removedLightDataIndices.erase(m_removedLightDataIndices.begin() + i);
			}
		}

		if (!m_invalidLightDataIndices.empty() || !m_removedLightDataIndices.empty())
		{
			ScatteredBufferUpload<LightDrawData> bufferUpload{ m_invalidLightDataIndices.size() + m_removedLightDataIndices.size() };

			for (const auto& invalidLight : m_invalidLightDataIndices)
			{
				auto& lightData = GetLightDataFromID(invalidLight.id);
				auto& data = bufferUpload.AddUploadItem(invalidLight.index);
				BuildSingleLightDrawData(data, lightData);
			}

			bufferUpload.UploadTo(renderGraph, m_buffers.lightsBuffer->GetResource());
			m_invalidLightDataIndices.clear();
			m_removedLightDataIndices.clear();
		}
	}

	VT_NODISCARD RenderLightData& RenderScene::GetLightDataFromID(UUID64 id)
	{
		auto it = std::ranges::find_if(m_renderLights, [id](const auto& light)
		{
			return light.id == id;
		});

		if (it == m_renderLights.end())
		{
			static RenderLightData nullObject;
			return nullObject;
		}

		return *it;
	}

	PrimitiveDrawData& RenderScene::GetPrimitiveDrawDataFromIndex(size_t index)
	{
		if (m_primitiveDrawData.size() <= index)
		{
			m_primitiveDrawData.resize(index + 1);
		}

		return m_primitiveDrawData[index];
	}

	size_t RenderScene::PrimitiveIndicesContainer::GetAvailableIndex(UUID64 id)
	{
		constexpr size_t SizeMax = std::numeric_limits<size_t>::max();
		size_t primitiveDrawDataIndex = SizeMax;

		if (!m_freePrimitiveDataIndices.empty())
		{
			primitiveDrawDataIndex = m_freePrimitiveDataIndices.back();
			m_freePrimitiveDataIndices.pop_back();

			auto it = std::ranges::find(m_removedPrimitiveDataIndices, primitiveDrawDataIndex);

			if (it != m_removedPrimitiveDataIndices.end())
			{
				m_removedPrimitiveDataIndices.erase_unsorted(it);
			}
		}

		const size_t newIndex = primitiveDrawDataIndex == SizeMax ? m_nextIndex++ : primitiveDrawDataIndex;
		m_primitiveIndexFromPrimitiveID[id] = newIndex;

		return newIndex;
	}

	void RenderScene::PrimitiveIndicesContainer::FreeIndexWithID(UUID64 id)
	{
		VT_ENSURE(m_primitiveIndexFromPrimitiveID.contains(id));

		const size_t index = m_primitiveIndexFromPrimitiveID.at(id);

		VT_ENSURE(std::ranges::find(m_freePrimitiveDataIndices, index) == m_freePrimitiveDataIndices.end());

		m_freePrimitiveDataIndices.emplace_back(index);
		m_removedPrimitiveDataIndices.emplace_back(index);
		m_primitiveIndexFromPrimitiveID.erase(id);

		auto it = std::ranges::find_if(m_invalidPrimitiveDataIndices, [index](const InvalidDrawData& data)
		{
			return data.index == index;
		});

		if (it != m_invalidPrimitiveDataIndices.end())
		{
			m_invalidPrimitiveDataIndices.erase_unsorted(it);
		}
	}

	void RenderScene::PrimitiveIndicesContainer::InvalidateIndexWithID(UUID64 id)
	{
		if (m_primitiveIndexFromPrimitiveID.contains(id))
		{
			size_t primitiveIndex = m_primitiveIndexFromPrimitiveID.at(id);

			auto invalidIt = std::ranges::find_if(m_invalidPrimitiveDataIndices, [primitiveIndex](const InvalidDrawData& drawData)
			{
				return drawData.index == primitiveIndex;
			});

			if (invalidIt != m_invalidPrimitiveDataIndices.end())
			{
				return;
			}

			m_invalidPrimitiveDataIndices.emplace_back(id, primitiveIndex);
		}
	}
}
