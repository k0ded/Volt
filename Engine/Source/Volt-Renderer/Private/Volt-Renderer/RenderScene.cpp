#include "vrpch.h"
#include "Volt-Renderer/RenderScene.h"

#include "Volt-Renderer/Material.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"
#include "Volt-Renderer/Texture/Texture2D.h"

#include <RenderCore/Shader/GlobalShader.h>

#include <Volt-Animation/MotionWeaver.h>
#include <Volt-Animation/Assets/Skeleton.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <EntitySystem/EntityScene.h>
#include <EntitySystem/EntityHelper.h>
#include <AssetSystem/AssetManager.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/RHIFeatures.h>

#include <ranges>

VT_DEFINE_LOG_CATEGORY(LogRenderScene);

namespace Volt
{
	static ConsoleVariable<int32_t> s_logRenderSceneUpdatedCVar("r.RenderScene.LogUpdates", 0, "Whether or not to log Render Scene updates");

	RenderScene::RenderScene(EntityScene* sceneRef)
		: m_scene(sceneRef)
	{
		m_buffers.meshesBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(GPUMesh), "GPU Meshes");
		m_buffers.sdfMeshesBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(GPUMeshSDF), "SDF GPU Meshes");
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
		}
	}

	RenderScene::~RenderScene()
	{
	}

	void RenderScene::Update(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		UpdateInvalidMaterials(renderGraph);
		UpdateInvalidMeshes(renderGraph);
		UpdateInvalidLights(renderGraph);
		UpdateInvalidPrimitiveData(renderGraph);
		CompactValidPrimitiveDrawDatas(renderGraph);

		// Temporary animation sampling
		m_currentBoneCount = 0;
		for (const auto& animatedObject : m_animatedRenderObjects)
		{
			auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndicesContainer.GetIndexFromID(animatedObject));
			primitiveDrawData.boneOffset = m_currentBoneCount;

			const auto& renderObject = GetPrimitiveDataFromID(animatedObject);
			m_currentBoneCount += static_cast<uint32_t>(renderObject.motionWeaver->GetSkeleton()->GetJointCount());
		}

		m_animationBufferStorage.resize(m_currentBoneCount);
		if (m_currentBoneCount > 0)
		{
			for (const auto& animatedObject : m_animatedRenderObjects)
			{
				const auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndicesContainer.GetIndexFromID(animatedObject));
				const auto& renderObject = GetPrimitiveDataFromID(animatedObject);

				const auto sample = renderObject.motionWeaver->Sample();
				memcpy_s(m_animationBufferStorage.data() + primitiveDrawData.boneOffset, sizeof(glm::mat4) * sample.size(), sample.data(), sizeof(glm::mat4) * sample.size());
			}
		}

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
	}

	void RenderScene::EndFrame(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		m_buffers.prevPrimitiveDrawDataBuffer->GrowIfRequired(m_buffers.primitiveDrawDataBuffer->GetResource()->GetCount());

		RGBufferRef srcPrimitiveData = renderGraph.RegisterExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource());
		RGBufferRef dstPrimitiveData = renderGraph.RegisterExternalBuffer(m_buffers.prevPrimitiveDrawDataBuffer->GetResource());

		AddCopyBufferPass(renderGraph, srcPrimitiveData, 0, dstPrimitiveData, 0, m_buffers.primitiveDrawDataBuffer->GetResource()->GetByteSize(), "Copy PrimitiveDrawData");
	}

	void RenderScene::InvalidatePrimitiveInstance(UUID64 renderObject)
	{
		m_primitiveIndicesContainer.InvalidateIndexWithID(renderObject);
	}

	void RenderScene::InvalidateMesh(Ref<Mesh> mesh)
	{
		std::scoped_lock lock{ m_meshUpdateMutex };
		for (uint32_t index = 0; index < mesh->GetNumSubMeshes(); index++)
		{
			const size_t hash = Math::HashCombine(mesh->GetHash(), std::hash<uint32_t>()(index));
			if (m_meshSubMeshToGPUMeshIndex.contains(hash))
			{
				m_invalidMeshes.emplace_back(mesh, index, m_meshSubMeshToGPUMeshIndex.at(hash));
			}
		}
	}

	void RenderScene::InvalidateMaterial(Ref<RenderMaterial> material)
	{
		std::scoped_lock lock{ m_materialUpdateMutex };
		const size_t hash = material->GetHash();
		if (m_gpuMaterialIndexFromMaterialHash.contains(hash))
		{
			m_invalidMaterials.emplace_back(material, m_gpuMaterialIndexFromMaterialHash.at(hash));
		}
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		UUID64 newId = {};
		auto& newObj = m_renderPrimitives.emplace_back();

		newObj.id = newId;
		newObj.entityId = entityId;
		newObj.mesh = mesh;
		newObj.material = material;
		newObj.subMeshIndex = subMeshIndex;

		TryAddMaterial(material);
		TryAddMesh(mesh);

		const size_t primitiveDrawDataIndex = m_primitiveIndicesContainer.GetAvailableIndex(newId);
		PrimitiveDrawData& primitiveDrawData = GetPrimitiveDrawDataFromIndex(primitiveDrawDataIndex);

		BuildSinglePrimitiveDrawData(primitiveDrawData, newObj);
		InvalidatePrimitiveInstance(newId);

		return newId;
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<MotionWeaver> motionWeaver, Ref<Mesh> mesh, Ref<RenderMaterial> material, uint32_t subMeshIndex)
	{
		UUID64 newId = {};
		auto& newObj = m_renderPrimitives.emplace_back();
		m_animatedRenderObjects.emplace_back(newId);

		newObj.id = newId;
		newObj.entityId = entityId;
		newObj.mesh = mesh;
		newObj.material = material;
		newObj.subMeshIndex = subMeshIndex;
		newObj.motionWeaver = motionWeaver;

		TryAddMaterial(material);
		TryAddMesh(mesh);

		size_t primitiveDrawDataIndex = m_primitiveIndicesContainer.GetAvailableIndex(newId);
		PrimitiveDrawData& primitiveDrawData = GetPrimitiveDrawDataFromIndex(primitiveDrawDataIndex);

		BuildSinglePrimitiveDrawData(primitiveDrawData, newObj);
		InvalidatePrimitiveInstance(newId);

		return newId;
	}

	void RenderScene::RemovePrimitiveInstance(UUID64 id)
	{
		m_primitiveIndicesContainer.FreeIndexWithID(id);

		auto it = std::find_if(m_renderPrimitives.begin(), m_renderPrimitives.end(), [id](const auto& obj)
		{
			return obj.id == id;
		});

		if (it == m_renderPrimitives.end())
		{
			return;
		}

		const bool isAnimated = (*it).IsAnimated();

		if (it != m_renderPrimitives.end())
		{
			m_renderPrimitives.erase(it);
		}

		if (isAnimated)
		{
			auto animIt = std::find_if(m_animatedRenderObjects.begin(), m_animatedRenderObjects.end(), [id](const auto& obj)
			{
				return obj == id;
			});

			if (animIt != m_animatedRenderObjects.end())
			{
				m_animatedRenderObjects.erase(animIt);
			}
		}
	}

	void RenderScene::InvalidateLightInstance(UUID64 id)
	{
		if (m_lightIndexFromLightID.contains(id))
		{
			m_invalidLightDataIndices.emplace_back(id, m_lightIndexFromLightID.at(id));
		}
	}

	UUID64 RenderScene::AddLightInstance(EntityID entityId, const SceneLightDescription& description)
	{
		UUID64 id{};

		auto& lightInstance = m_renderLights.emplace_back();
		lightInstance.id = id;
		lightInstance.entityId = entityId;
		lightInstance.description = description;

		constexpr size_t SizeMax = std::numeric_limits<size_t>::max();

		size_t lightDrawDataIndex = SizeMax;

		if (!m_freeLightDataIndices.empty())
		{
			lightDrawDataIndex = m_freeLightDataIndices.back();
			m_freeLightDataIndices.pop_back();
		}

		LightDrawData& lightDrawData = (lightDrawDataIndex != SizeMax) ? m_lightDrawData.at(lightDrawDataIndex) : m_lightDrawData.emplace_back();
		lightDrawData.lightType = description.lightType;

		m_lightIndexFromLightID[id] = static_cast<uint32_t>((lightDrawDataIndex != SizeMax) ? lightDrawDataIndex : m_lightDrawData.size() - 1);
		InvalidateLightInstance(id);

		return id;
	}

	void RenderScene::RemoveLightInstance(UUID64 id)
	{
		VT_ENSURE(m_lightIndexFromLightID.contains(id));

		const size_t lightDataIndex = m_lightIndexFromLightID.at(id);

		m_lightDrawData.at(lightDataIndex).flags = LightFlags::Invalid;
		m_removedLightDataIndices.emplace_back(lightDataIndex);
		m_freeLightDataIndices.emplace_back(lightDataIndex);

		m_lightIndexFromLightID.erase(id);

		auto it = std::ranges::find_if(m_renderLights, [id](const auto& obj)
		{
			return obj.id == id;
		});

		if (it != m_renderLights.end())
		{
			m_renderLights.erase(it);
		}
	}

	Weak<RenderMaterial> RenderScene::GetMaterialFromID(const uint32_t materialId) const
	{
		if (static_cast<size_t>(materialId) >= m_individualMaterials.size())
		{
			return {};
		}

		return m_individualMaterials.at(materialId);
	}

	const uint32_t RenderScene::GetMeshID(Weak<Mesh> mesh, uint32_t subMeshIndex) const
	{
		const size_t hash = Math::HashCombine(mesh->GetHash(), std::hash<uint32_t>()(subMeshIndex));
		return m_meshSubMeshToGPUMeshIndex.contains(hash) ? m_meshSubMeshToGPUMeshIndex.at(hash) : std::numeric_limits<uint32_t>::max();
	}

	const uint32_t RenderScene::GetMaterialIndex(Weak<RenderMaterial> material) const
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

	const uint32_t RenderScene::GetMeshIndex(Weak<Mesh> mesh) const
	{
		auto it = std::find_if(m_individualMeshes.begin(), m_individualMeshes.end(), [&](Weak<Mesh> lhs)
		{
			return lhs.Get() == mesh.Get();
		});

		if (it != m_individualMeshes.end())
		{
			return static_cast<uint32_t>(std::distance(m_individualMeshes.begin(), it));
		}

		return std::numeric_limits<uint32_t>::max();
	}

	const uint32_t RenderScene::GetPrimitiveIndexFromID(UUID64 primitiveId) const
	{
		return static_cast<uint32_t>(m_primitiveIndicesContainer.GetIndexFromID(primitiveId));
	}

	GPUSceneParameters RenderScene::GetGPUSceneParameters(RenderGraph& renderGraph) const
	{
		GPUSceneParameters result;
		result.PrimitiveDrawDataBuffer = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource()));
		result.PrevPrimitiveDrawDataBuffer = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.prevPrimitiveDrawDataBuffer->GetResource()));
		result.SceneLights = renderGraph.CreateSRV(renderGraph.RegisterExternalBuffer(m_buffers.lightsBuffer->GetResource()));

		return result;
	}

	const RenderPrimitiveData& RenderScene::GetPrimitiveDataFromID(UUID64 id) const
	{
		auto it = std::ranges::find_if(m_renderPrimitives, [id](const auto& renderObject)
		{
			return renderObject.id == id;
		});

		if (it == m_renderPrimitives.end())
		{
			static RenderPrimitiveData nullObject;
			return nullObject;
		}

		return *it;
	}

	VT_NODISCARD const RenderLightData& RenderScene::GetLightDataFromID(UUID64 id) const
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

	VT_NODISCARD PagedVector<uint32_t> RenderScene::GetPrimitiveIndicesFromEntityID(EntityID entityId) const
	{
		PagedVector<uint32_t> result;

		for (const auto& primitive : m_renderPrimitives)
		{
			if (primitive.entityId == entityId)
			{
				result.emplace_back(static_cast<uint32_t>(m_primitiveIndicesContainer.GetIndexFromID(primitive.id)));
			}
		}

		return result;
	}

	void RenderScene::BuildGPUMaterial(Weak<RenderMaterial> material, GPUMaterial& gpuMaterial)
	{
		gpuMaterial.textureCount = 0;

		for (const auto& texture : material->GetTextures())
		{
			gpuMaterial.textures[gpuMaterial.textureCount] = texture.GetResource();
			// #TODO_Ivar: Bindless-support
			//gpuMaterial.samplers[gpuMaterial.textureCount] = SamplerStateCache::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::X16>()->GetResourceHandle();
			gpuMaterial.textureCount++;
		}
	}

	void RenderScene::BuildSinglePrimitiveDrawData(PrimitiveDrawData& primitiveDrawData, const RenderPrimitiveData& renderObject)
	{
		EntityHelper entity = m_scene->GetEntityHelperFromEntityID(renderObject.entityId);
		if (!entity.IsValid())
		{
			return;
		}

		const size_t hash = Math::HashCombine(renderObject.mesh->GetHash(), std::hash<uint32_t>()(renderObject.subMeshIndex));
		const uint32_t meshId = m_meshSubMeshToGPUMeshIndex.contains(hash) ? m_meshSubMeshToGPUMeshIndex.at(hash) : 0;

		primitiveDrawData.position = entity.GetPosition();
		primitiveDrawData.scale = entity.GetScale();
		primitiveDrawData.rotation = entity.GetRotation();
		primitiveDrawData.meshId = meshId;
		primitiveDrawData.entityId = entity.GetID();
		primitiveDrawData.materialId = GetMaterialIndex(renderObject.material);
		primitiveDrawData.meshletStartOffset = renderObject.meshletStartOffset;
		primitiveDrawData.isAnimated = renderObject.IsAnimated();
		primitiveDrawData.flags = PrimitiveFlags::Valid;

		m_currentMeshletCount += m_gpuMeshes.at(meshId).meshletCount;
	}

	void RenderScene::BuildSingleSDFPrimitiveDrawData(SDFPrimitiveDrawData& primtiveDrawData, const RenderPrimitiveData& renderObject)
	{
		EntityHelper entity = m_scene->GetEntityHelperFromEntityID(renderObject.entityId);
		if (!entity.IsValid())
		{
			return;
		}

		const size_t hash = Math::HashCombine(renderObject.mesh->GetHash(), std::hash<uint32_t>()(renderObject.subMeshIndex));
		const uint32_t meshId = m_meshSubMeshToGPUMeshSDFIndex.contains(hash) ? m_meshSubMeshToGPUMeshSDFIndex.at(hash) : 0;

		primtiveDrawData.position = entity.GetPosition();
		primtiveDrawData.scale = entity.GetScale();
		primtiveDrawData.rotation = entity.GetRotation();
		primtiveDrawData.primtiveId = static_cast<uint32_t>(m_primitiveIndicesContainer.GetIndexFromID(renderObject.id));
		primtiveDrawData.meshSDFId = meshId;
	}

	void RenderScene::BuildSingleLightDrawData(LightDrawData& lightDrawData, RenderLightData& renderLight)
	{
		EntityHelper entity = m_scene->GetEntityHelperFromEntityID(renderLight.entityId);
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

		m_individualMeshes.emplace_back(mesh);
		m_currentIndividualMeshCount += static_cast<uint32_t>(mesh->GetSubMeshes().size());

		size_t currentIndex = m_gpuMeshes.size();

		std::scoped_lock lock{ m_meshUpdateMutex };
		for (uint32_t subMeshIndex = 0; const auto& gpuMesh : mesh->GetGPUMeshes())
		{
			m_gpuMeshes.emplace_back(gpuMesh);

			const size_t meshHash = Math::HashCombine(mesh->GetHash(), std::hash<uint32_t>()(subMeshIndex));
			m_meshSubMeshToGPUMeshIndex[meshHash] = static_cast<uint32_t>(currentIndex);
			m_invalidMeshes.emplace_back(mesh, subMeshIndex, currentIndex);

			currentIndex++;
			subMeshIndex++;
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

		std::scoped_lock lock{ m_materialUpdateMutex };
		m_individualMaterials.emplace_back(material);
		m_gpuMaterialIndexFromMaterialHash[material->GetHash()] = gpuMaterialIndex;
		m_invalidMaterials.emplace_back(material, gpuMaterialIndex);
	
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

		std::scoped_lock lock{ m_materialUpdateMutex };
		for (const auto& material : m_individualMaterials)
		{
			if (material->DoMaterialRequireUpdate())
			{
				m_invalidMaterials.emplace_back(material, m_gpuMaterialIndexFromMaterialHash.at(material->GetHash()));
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

		std::scoped_lock lock{ m_meshUpdateMutex };

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
				data = invalidMesh.mesh->GetGPUMeshes().at(invalidMesh.subMeshIndex);

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

		PagedVector<size_t> removedPrimitiveDataIndices = m_primitiveIndicesContainer.GetAndClearRemovedIndices();
		PagedVector<InvalidDrawData> invalidPrimitiveDataIndices = m_primitiveIndicesContainer.GetAndClearInvalidIndices();

		if (!invalidPrimitiveDataIndices.empty() || !removedPrimitiveDataIndices.empty())
		{
			ScatteredBufferUpload<PrimitiveDrawData> bufferUpload{ invalidPrimitiveDataIndices.size() + removedPrimitiveDataIndices.size() };

			for (const auto& invalidPrimitive : invalidPrimitiveDataIndices)
			{
				const auto& renderObject = GetPrimitiveDataFromID(invalidPrimitive.id);
				auto& data = bufferUpload.AddUploadItem(invalidPrimitive.index);
				BuildSinglePrimitiveDrawData(data, renderObject);

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Primitive Data attached to entity {} was uploaded to index {}.", data.entityId, invalidPrimitive.index);
				}
			}

			for (const auto& removedPrimitiveIndex : removedPrimitiveDataIndices)
			{
				auto& data = bufferUpload.AddUploadItem(removedPrimitiveIndex);
				data.flags = PrimitiveFlags::Invalid;

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
	REGISTER_SHADER(CompactValidDrawCallCS, "Engine/Shaders/Source/RenderScene/CompactValidDrawCalls.hlsl", "MainCS", Compute);

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
