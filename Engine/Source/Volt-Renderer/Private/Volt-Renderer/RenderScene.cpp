#include "vrpch.h"
#include "Volt-Renderer/RenderScene.h"

#include "Volt-Renderer/Material.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"
#include "Volt-Renderer/Utility/ScatteredBufferUpload.h"
#include "Volt-Renderer/Texture/Texture2D.h"

#include <Volt-Animation/MotionWeaver.h>
#include <Volt-Animation/Assets/Skeleton.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <EntitySystem/EntityScene.h>
#include <EntitySystem/EntityHelper.h>
#include <AssetSystem/AssetManager.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Graphics/GraphicsContext.h>

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
		m_buffers.sdfPrimitiveDrawDataBuffer = CreateRef<GrowingGPUBuffer>(5, sizeof(SDFPrimitiveDrawData), "SDF Primitive Draw Data");
		m_buffers.bonesBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(glm::mat4), "GPU Bones");
		m_buffers.lightsBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(LightDrawData), "Lights");
		m_buffers.validPrimitiveDrawDatasBuffer = CreateRef<GrowingGPUBuffer>(1, sizeof(uint32_t), "Compacted Valid Primitive Draw Datas");

		// Setup invalid mesh
		{
			GPUMesh& mesh = m_gpuMeshes.emplace_back();
			memset(&mesh, 0, sizeof(GPUMesh));
		}

		m_materialChangedCallbackID = AssetManager::RegisterAssetChangedCallback(AssetTypes::Material, [&](AssetHandle handle, AssetChangedState state)
		{
			if (!m_materialIndexFromAssetHandle.contains(handle) || state != AssetChangedState::Updated)
			{
				return;
			}

			Ref<Material> material = AssetManager::GetAsset<Material>(handle);
			m_invalidMaterials.emplace_back(material, m_materialIndexFromAssetHandle.at(handle));
		});

		m_meshChangedCallbackID = AssetManager::RegisterAssetChangedCallback(AssetTypes::Mesh, [&](AssetHandle handle, AssetChangedState state)
		{
			if (state != AssetChangedState::Updated)
			{
				return;
			}

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(handle);
			for (uint32_t subMeshIndex = 0; subMeshIndex < static_cast<uint32_t>(mesh->GetSubMeshes().size()); subMeshIndex++)
			{
				const size_t assetHash = Math::HashCombine(handle, std::hash<uint32_t>()(subMeshIndex));
				if (m_gpuMeshIndexFromMeshAssetHash.contains(assetHash))
				{
					m_invalidMeshes.emplace_back(mesh, subMeshIndex, m_gpuMeshIndexFromMeshAssetHash.at(assetHash));
				}
			}
		});

		if (RHI::GraphicsContext::GetDevice()->GetCapabilities().rayTracing.supportsRayTracing)
		{
			m_rayTracingScene = CreateRef<RayTracingScene>(m_scene);
		}
	}

	RenderScene::~RenderScene()
	{
		AssetManager::UnregisterAssetChangedCallback(AssetTypes::Material, m_materialChangedCallbackID);
		AssetManager::UnregisterAssetChangedCallback(AssetTypes::Mesh, m_meshChangedCallbackID);
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
			auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndexFromPrimitiveID.at(animatedObject));
			primitiveDrawData.boneOffset = m_currentBoneCount;

			const auto& renderObject = GetPrimitiveDataFromID(animatedObject);
			m_currentBoneCount += static_cast<uint32_t>(renderObject.motionWeaver->GetSkeleton()->GetJointCount());
		}

		m_animationBufferStorage.resize(m_currentBoneCount);
		if (m_currentBoneCount > 0)
		{
			for (const auto& animatedObject : m_animatedRenderObjects)
			{
				const auto& primitiveDrawData = m_primitiveDrawData.at(m_primitiveIndexFromPrimitiveID.at(animatedObject));
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

		if (RHI::GraphicsContext::GetDevice()->GetCapabilities().rayTracing.supportsRayTracing)
		{
			m_rayTracingScene->Update();
		}
	}

	void RenderScene::EndFrame(RenderGraph& renderGraph)
	{
		m_buffers.prevPrimitiveDrawDataBuffer->GrowIfRequired(m_buffers.primitiveDrawDataBuffer->GetResource()->GetCount());
	
		RGUtils::CopyBuffer(renderGraph,
			renderGraph.AddExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource()),
			renderGraph.AddExternalBuffer(m_buffers.prevPrimitiveDrawDataBuffer->GetResource()),
			m_buffers.primitiveDrawDataBuffer->GetResource()->GetByteSize(),
			"Copy PrimitiveDrawData");
	}

	void RenderScene::InvalidatePrimitiveInstance(UUID64 renderObject)
	{
		if (m_primitiveIndexFromPrimitiveID.contains(renderObject))
		{
			m_invalidPrimitiveDataIndices.emplace_back(renderObject, m_primitiveIndexFromPrimitiveID.at(renderObject));
		}

		if (m_sdfPrimitiveIndexFromPrimitiveID.contains(renderObject))
		{
			m_invalidSDFPrimitiveDataIndices.emplace_back(renderObject, m_sdfPrimitiveIndexFromPrimitiveID.at(renderObject));
		}
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<Mesh> mesh, Ref<Material> material, uint32_t subMeshIndex)
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

		constexpr size_t SizeMax = std::numeric_limits<size_t>::max();

		size_t primitiveDrawDataIndex = SizeMax;

		if (!m_freePrimitiveDataIndices.empty())
		{
			primitiveDrawDataIndex = m_freePrimitiveDataIndices.back();
			m_freePrimitiveDataIndices.pop_back();
		}

		PrimitiveDrawData& primitiveDrawData = (primitiveDrawDataIndex != SizeMax) ? m_primitiveDrawData.at(primitiveDrawDataIndex) : m_primitiveDrawData.emplace_back();

		BuildSinglePrimitiveDrawData(primitiveDrawData, newObj);
		m_primitiveIndexFromPrimitiveID[newObj.id] = static_cast<uint32_t>((primitiveDrawDataIndex != SizeMax) ? primitiveDrawDataIndex : m_primitiveDrawData.size() - 1);

		InvalidatePrimitiveInstance(newId);

		return newId;
	}

	UUID64 RenderScene::AddPrimitiveInstance(EntityID entityId, Ref<MotionWeaver> motionWeaver, Ref<Mesh> mesh, Ref<Material> material, uint32_t subMeshIndex)
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

		constexpr size_t SizeMax = std::numeric_limits<size_t>::max();

		size_t primitiveDrawDataIndex = SizeMax;

		if (!m_freePrimitiveDataIndices.empty())
		{
			primitiveDrawDataIndex = m_freePrimitiveDataIndices.back();
			m_freePrimitiveDataIndices.pop_back();
		}

		PrimitiveDrawData& primitiveDrawData = (primitiveDrawDataIndex != SizeMax) ? m_primitiveDrawData.at(primitiveDrawDataIndex) : m_primitiveDrawData.emplace_back();

		BuildSinglePrimitiveDrawData(primitiveDrawData, newObj);
		m_primitiveIndexFromPrimitiveID[newObj.id] = static_cast<uint32_t>((primitiveDrawDataIndex != SizeMax) ? primitiveDrawDataIndex : m_primitiveDrawData.size() - 1);

		InvalidatePrimitiveInstance(newId);

		return newId;
	}

	void RenderScene::RemovePrimitiveInstance(UUID64 id)
	{
		VT_ENSURE(m_primitiveIndexFromPrimitiveID.contains(id));

		const size_t primitiveDataIndex = m_primitiveIndexFromPrimitiveID.at(id);

		m_primitiveDrawData.at(primitiveDataIndex).flags = PrimitiveFlags::Invalid;
		m_removedPrimitiveDataIndices.emplace_back(primitiveDataIndex);
		m_freePrimitiveDataIndices.emplace_back(primitiveDataIndex);

		m_primitiveIndexFromPrimitiveID.erase(id);

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

	Weak<Material> RenderScene::GetMaterialFromID(const uint32_t materialId) const
	{
		if (static_cast<size_t>(materialId) >= m_individualMaterials.size())
		{
			return {};
		}

		return m_individualMaterials.at(materialId);
	}

	const uint32_t RenderScene::GetMeshID(Weak<Mesh> mesh, uint32_t subMeshIndex) const
	{
		const size_t hash = Math::HashCombine(mesh.GetHash(), std::hash<uint32_t>()(subMeshIndex));
		return m_meshSubMeshToGPUMeshIndex.contains(hash) ? m_meshSubMeshToGPUMeshIndex.at(hash) : std::numeric_limits<uint32_t>::max();
	}

	const uint32_t RenderScene::GetMaterialIndex(Weak<Material> material) const
	{
		auto it = std::find_if(m_individualMaterials.begin(), m_individualMaterials.end(), [&](Weak<Material> lhs)
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

	VT_NODISCARD const uint32_t RenderScene::GetPrimitiveIndexFromID(UUID64 primitiveId) const
	{
		VT_ENSURE(m_primitiveIndexFromPrimitiveID.contains(primitiveId));
		return m_primitiveIndexFromPrimitiveID.at(primitiveId);
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

	void RenderScene::BuildGPUMaterial(Weak<Material> material, GPUMaterial& gpuMaterial)
	{
		gpuMaterial.textureCount = 0;
		if (!material->IsValid())
		{
			material = Renderer::GetDefaultResources().defaultMaterial;
		}

		for (const auto& texture : material->GetTextures())
		{
			ResourceHandle textureHandle = ResourceHandle(0u);
			if (texture->IsValid())
			{
				textureHandle = texture->GetResourceHandle();
			}

			gpuMaterial.textures[gpuMaterial.textureCount] = textureHandle;
			gpuMaterial.samplers[gpuMaterial.textureCount] = Renderer::GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::X16>()->GetResourceHandle();
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

		const size_t hash = Math::HashCombine(renderObject.mesh.GetHash(), std::hash<uint32_t>()(renderObject.subMeshIndex));
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

		const size_t hash = Math::HashCombine(renderObject.mesh.GetHash(), std::hash<uint32_t>()(renderObject.subMeshIndex));
		const uint32_t meshId = m_meshSubMeshToGPUMeshSDFIndex.contains(hash) ? m_meshSubMeshToGPUMeshSDFIndex.at(hash) : 0;

		primtiveDrawData.position = entity.GetPosition();
		primtiveDrawData.scale = entity.GetScale();
		primtiveDrawData.rotation = entity.GetRotation();
		primtiveDrawData.primtiveId = m_primitiveIndexFromPrimitiveID.at(renderObject.id);
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

		for (uint32_t subMeshIndex = 0; const auto & gpuMesh : mesh->GetGPUMeshes())
		{
			m_gpuMeshes.emplace_back(gpuMesh);

			const size_t hash = Math::HashCombine(std::hash<void*>()(mesh.get()), std::hash<uint32_t>()(subMeshIndex));
			m_meshSubMeshToGPUMeshIndex[hash] = static_cast<uint32_t>(currentIndex);

			const size_t assetHash = Math::HashCombine(mesh->handle, std::hash<uint32_t>()(subMeshIndex));
			m_gpuMeshIndexFromMeshAssetHash[assetHash] = currentIndex;

			m_invalidMeshes.emplace_back(mesh, subMeshIndex, m_gpuMeshIndexFromMeshAssetHash.at(assetHash));

			currentIndex++;
			subMeshIndex++;
		}
	}

	void RenderScene::TryAddMaterial(Ref<Material> material)
	{
		VT_PROFILE_FUNCTION();

		auto it = std::find(m_individualMaterials.begin(), m_individualMaterials.end(), material);
		if (it != m_individualMaterials.end())
		{
			return;
		}

		m_individualMaterials.emplace_back(material);
		m_materialIndexFromAssetHandle[material->handle] = m_gpuMaterials.size();

		GPUMaterial& gpuMaterial = m_gpuMaterials.emplace_back();
		BuildGPUMaterial(material, gpuMaterial);

		m_invalidMaterials.emplace_back(material, m_materialIndexFromAssetHandle[material->handle]);
	}

	void RenderScene::UpdateInvalidMaterials(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		auto materialsBuffer = m_buffers.materialsBuffer;
		materialsBuffer->GrowIfRequired(m_individualMaterials.size());

		for (const auto& material : m_individualMaterials)
		{
			if (material->ClearAndGetIsDirty())
			{
				m_invalidMaterials.emplace_back(material, m_materialIndexFromAssetHandle.at(material->handle));
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
					VT_LOGC(Trace, LogRenderScene, "Material {} was uploaded to index {}.", invalidMaterial.material->assetName, invalidMaterial.index);
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
					VT_LOGC(Trace, LogRenderScene, "Mesh {} was uploaded to index {}.", invalidMesh.mesh->assetName, invalidMesh.index);
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

		if (!m_invalidPrimitiveDataIndices.empty() || !m_removedPrimitiveDataIndices.empty())
		{
			ScatteredBufferUpload<PrimitiveDrawData> bufferUpload{ m_invalidPrimitiveDataIndices.size() + m_removedPrimitiveDataIndices.size() };

			for (const auto& invalidPrimitive : m_invalidPrimitiveDataIndices)
			{
				const auto& renderObject = GetPrimitiveDataFromID(invalidPrimitive.id);
				auto& data = bufferUpload.AddUploadItem(invalidPrimitive.index);
				BuildSinglePrimitiveDrawData(data, renderObject);

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Primitive Data attached to entity {} was uploaded to index {}.", data.entityId, invalidPrimitive.index);
				}
			}

			for (const auto& removedPrimitiveIndex : m_removedPrimitiveDataIndices)
			{
				auto& data = bufferUpload.AddUploadItem(removedPrimitiveIndex);
				data.flags = PrimitiveFlags::Invalid;

				if (s_logRenderSceneUpdatedCVar.GetValue())
				{
					VT_LOGC(Trace, LogRenderScene, "Primitive Data with index {} was removed.", removedPrimitiveIndex);
				}
			}

			bufferUpload.UploadTo(renderGraph, drawDataBuffer->GetResource());
			m_invalidPrimitiveDataIndices.clear();
			m_removedPrimitiveDataIndices.clear();
		}

		m_buffers.sdfPrimitiveDrawDataBuffer->GrowIfRequired(m_sdfPrimitiveDrawData.size());

		if (!m_invalidSDFPrimitiveDataIndices.empty())
		{
			ScatteredBufferUpload<SDFPrimitiveDrawData> bufferUpload{ m_invalidSDFPrimitiveDataIndices.size() };

			for (const auto& invalidPrimitive : m_invalidSDFPrimitiveDataIndices)
			{
				const auto& renderObject = GetPrimitiveDataFromID(invalidPrimitive.id);
				auto& data = bufferUpload.AddUploadItem(invalidPrimitive.index);
				BuildSingleSDFPrimitiveDrawData(data, renderObject);
			}

			bufferUpload.UploadTo(renderGraph, m_buffers.sdfPrimitiveDrawDataBuffer->GetResource());
			m_invalidSDFPrimitiveDataIndices.clear();
		}
	}

	void RenderScene::CompactValidPrimitiveDrawDatas(RenderGraph& renderGraph)
	{
		// We need to make sure that the buffer is one larger than the count, because
		// the first index is used for the count.
		auto validPrimitiveDrawDataBuffer = m_buffers.validPrimitiveDrawDatasBuffer;
		
		const uint32_t primitiveDrawDataCount = m_buffers.primitiveDrawDataBuffer->GetResource()->GetCount();
		validPrimitiveDrawDataBuffer->GrowIfRequired(primitiveDrawDataCount + 1);

		RenderGraphBufferHandle validPrimitiveDrawDataHandle = renderGraph.AddExternalBuffer(validPrimitiveDrawDataBuffer->GetResource());
		RenderGraphBufferHandle primitiveDrawDataHandle = renderGraph.AddExternalBuffer(m_buffers.primitiveDrawDataBuffer->GetResource());

		RGUtils::ClearBuffer(renderGraph, validPrimitiveDrawDataHandle, 0, "Clear Valid Primitive Draw Data Buffer");

		renderGraph.AddPass("Compact Valid Primitive Draw Datas",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(validPrimitiveDrawDataHandle);
			builder.ReadResource(primitiveDrawDataHandle);

			builder.SetIsComputePass();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline("CompactValidDrawCalls");

			context.BindPipeline(pipeline);
			context.SetConstant("validPrimitiveDrawData"_sh, validPrimitiveDrawDataHandle);
			context.SetConstant("primitiveDrawData"_sh, primitiveDrawDataHandle);
			context.SetConstant("primitiveDrawDataCount"_sh, primitiveDrawDataCount);

			constexpr uint32_t workGroupCount = 64;

			context.Dispatch(Math::DivideRoundUp(primitiveDrawDataCount, workGroupCount), 1, 1);
		});
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
}
