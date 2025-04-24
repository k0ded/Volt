#include "vrpch.h"
#include "Volt-Renderer/Renderer.h"

#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/RenderMaterial.h"
#include "Volt-Renderer/ShapeLibrary.h"

#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetManager.h>
#include <JobSystem/TaskGraph.h>

#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContextUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Debug/ShaderRuntimeValidator.h>
#include <RenderCore/Resources/BindlessResourcesManager.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>

#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Descriptors/DescriptorTable.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(Renderer, Engine, 3);

	struct EquirectangularToCubemapCS
	{
		BEGIN_SHADER_DEFINITION(EquirectangularToCubemapCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Environment/EquirectangularToCubemap.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()
	};
	REGISTER_SHADER(EquirectangularToCubemapCS)

	struct IntegrateSpecularCubeCS
	{
		BEGIN_SHADER_DEFINITION(IntegrateSpecularCubeCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PBR/IntegrateSpecularCube.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()
	};
	REGISTER_SHADER(IntegrateSpecularCubeCS)

	struct IntegrateDiffuseCubeCS
	{
		BEGIN_SHADER_DEFINITION(IntegrateDiffuseCubeCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PBR/IntegrateDiffuseCube.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()
	};
	REGISTER_SHADER(IntegrateDiffuseCubeCS)

	struct GeneratePreIntegratedDFG
	{
		BEGIN_SHADER_DEFINITION(GeneratePreIntegratedDFG)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/Utility/FullscreenTriangle_vs.hlsl", "main", RHI::ShaderStage::Vertex)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/PBR/GeneratePreIntegratedDFG.hlsl", "MainPS", RHI::ShaderStage::Pixel)
		END_SHADER_DEFINITION()
	};
	REGISTER_SHADER(GeneratePreIntegratedDFG)

	namespace Utility
	{
		inline static const size_t GetHashFromSamplerInfo(const RHI::SamplerStateCreateInfo& info)
		{
			size_t hash = std::hash<uint32_t>()(static_cast<uint32_t>(info.minFilter));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.magFilter)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.mipFilter)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.wrapMode)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.compareOperator)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.anisotropyLevel)));
			hash = Math::HashCombine(hash, std::hash<float>()(info.mipLodBias));
			hash = Math::HashCombine(hash, std::hash<float>()(info.minLod));
			hash = Math::HashCombine(hash, std::hash<float>()(info.maxLod));

			return hash;
		}
	}

	struct RendererData
	{
		~RendererData()
		{
			defaultResources.Clear();
			samplers.clear();

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
			shaderValidator = nullptr;
#endif
			bindlessResourcesManager = nullptr;

			for (auto& resourceQueue : deletionQueue)
			{
				resourceQueue.Flush();
			}
		}

		Scope<BindlessResourcesManager> bindlessResourcesManager;

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
		Scope<ShaderRuntimeValidator> shaderValidator;
#endif

		Vector<FunctionQueue> deletionQueue;
		std::unordered_map<size_t, BindlessResourceRef<RHI::SamplerState>> samplers;

		DefaultResources defaultResources;
	};

	Renderer::Renderer()
	{
		VT_ASSERT(!s_instance);
		s_instance = this;

		RegisterListener<AppPostFrameUpdateEvent>(VT_BIND_EVENT_FN(Renderer::OnEndOfFrameUpdate));
		RegisterListener<AppPreRenderEvent>(VT_BIND_EVENT_FN(Renderer::OnPreRenderEvent));
	}

	Renderer::~Renderer()
	{
		s_instance = nullptr;
	}

	void Renderer::Initialize()
	{
		m_deletionQueue.resize(WindowManager::Get().GetMainWindow().GetSwapchain().GetFramesInFlight());

		// Bindless resources manager
		{
			m_bindlessResourcesManager = CreateScope<BindlessResourcesManager>();
		}

		RenderGraphExecutionThread::Initialize(RenderGraphExecutionThread::ExecutionMode::Multithreaded);

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
		m_shaderValidator = CreateScope<ShaderRuntimeValidator>();
#endif

		CreateDefaultResources();
		m_blueNoise = CreateScope<BlueNoise>();
	}

	void Renderer::Shutdown()
	{
		RenderGraphExecutionThread::Shutdown();

		m_blueNoise.reset();

		m_defaultResources.Clear();
		m_samplers.clear();

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
		m_shaderValidator = nullptr;
#endif

		m_shaderMap = nullptr;

		for (auto& resourceQueue : m_deletionQueue)
		{
			resourceQueue.Flush();
		}

		m_bindlessResourcesManager = nullptr;

		for (auto& resourceQueue : m_deletionQueue)
		{
			resourceQueue.Flush();
		}
	}

	const uint32_t Renderer::GetFramesInFlight()
	{
		return WindowManager::Get().GetMainWindow().GetSwapchain().GetFramesInFlight();
	}

	void Renderer::DestroyResource(std::function<void()>&& function)
	{
		if (!s_instance)
		{
			function();
			return;
		}

		const uint32_t currentFrame = WindowManager::Get().GetMainWindow().GetSwapchain().GetCurrentFrame();
		s_instance->m_deletionQueue.at(currentFrame).Push(std::move(function));
	}

	const DefaultResources& Renderer::GetDefaultResources()
	{
		return s_instance->m_defaultResources;
	}

	Renderer::EnvironmentTextures Renderer::GenerateEnvironmentTextures(AssetHandle baseTextureHandle)
	{
		Ref<Texture2D> environmentTexture = AssetManager::GetAsset<Texture2D>(baseTextureHandle);
		if (!environmentTexture || !environmentTexture->IsValid())
		{
			return {};
		}

		constexpr uint32_t CUBE_MAP_SIZE = 2048;
		constexpr uint32_t DIFFUSE_MAP_SIZE = 256;
		constexpr uint32_t CONVERSION_THREAD_GROUP_SIZE = 32;

		RefPtr<RHI::Image> environmentRaw;
		RefPtr<RHI::Image> environmentSpecular;
		RefPtr<RHI::Image> environmentDiffuse;

		auto linearSampler = GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>();

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		commandBuffer->Begin();

		// Unfiltered - Conversion
		{
			RHI::ImageSpecification imageSpec{};
			imageSpec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			imageSpec.width = CUBE_MAP_SIZE;
			imageSpec.height = CUBE_MAP_SIZE;
			imageSpec.usage = RHI::ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;

			environmentRaw = RHI::Image::Create(imageSpec);

			{
				RHI::ResourceBarrierInfo barrierInfo{};
				barrierInfo.type = RHI::BarrierType::Image;
				barrierInfo.imageBarrier().srcStage = RHI::BarrierStage::None;
				barrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::None;
				barrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::Undefined;
				barrierInfo.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				barrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
				barrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderWrite;
				barrierInfo.imageBarrier().resource = environmentRaw;
				commandBuffer->ResourceBarrier({ barrierInfo });
			}

			auto conversionPipeline = ShaderMap::GetComputePipeline<EquirectangularToCubemapCS>(false);

			RHI::DescriptorTableCreateInfo tableInfo{};
			tableInfo.shader = conversionPipeline->GetShader();

			RefPtr<RHI::DescriptorTable> descriptorTable = RHI::DescriptorTable::Create(tableInfo);
			descriptorTable->SetImageView("o_output", environmentRaw->GetArrayView(), 0);
			descriptorTable->SetImageView("u_equirectangularMap", environmentTexture->GetImage()->GetView(), 0);
			descriptorTable->SetSamplerState("u_linearSampler", linearSampler->GetResource(), 0);

			commandBuffer->BindPipeline(conversionPipeline);
			commandBuffer->BindDescriptorTable(descriptorTable);

			const uint32_t groupCount = Math::DivideRoundUp(CUBE_MAP_SIZE, CONVERSION_THREAD_GROUP_SIZE);
			commandBuffer->Dispatch(groupCount, groupCount, 6);

			{
				RHI::ResourceBarrierInfo imageBarrierInfo{};
				imageBarrierInfo.type = RHI::BarrierType::Image;
				imageBarrierInfo.imageBarrier().srcStage = RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
				imageBarrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::ShaderWrite;
				imageBarrierInfo.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				imageBarrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
				imageBarrierInfo.imageBarrier().resource = environmentRaw;

				RHI::ResourceBarrierInfo barrierInfo{};
				barrierInfo.type = RHI::BarrierType::Global;
				barrierInfo.globalBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
				barrierInfo.globalBarrier().srcStage = RHI::BarrierStage::ComputeShader;
				barrierInfo.globalBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				barrierInfo.globalBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				commandBuffer->ResourceBarrier({ barrierInfo, imageBarrierInfo });
			}
		}

		// Specular
		{
			RHI::ImageSpecification imageSpec{};
			imageSpec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			imageSpec.width = CUBE_MAP_SIZE;
			imageSpec.height = CUBE_MAP_SIZE;
			imageSpec.usage = RHI::ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;
			imageSpec.mips = RHI::Utility::CalculateMipCount(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
			imageSpec.debugName = "Environment - Specular";

			environmentSpecular = RHI::Image::Create(imageSpec);

			for (uint32_t i = 0; i < imageSpec.mips; i++)
			{
				environmentSpecular->GetArrayView(i);
			}

			{
				RHI::ResourceBarrierInfo barrierInfo{};
				barrierInfo.type = RHI::BarrierType::Image;
				barrierInfo.imageBarrier().srcStage = RHI::BarrierStage::None;
				barrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::None;
				barrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::Undefined;
				barrierInfo.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				barrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
				barrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderWrite;
				barrierInfo.imageBarrier().resource = environmentSpecular;
				commandBuffer->ResourceBarrier({ barrierInfo });
			}

			auto pipeline = ShaderMap::GetComputePipeline<IntegrateSpecularCubeCS>(false);
			RHI::DescriptorTableCreateInfo tableInfo{};
			tableInfo.shader = pipeline->GetShader();

			Vector<RefPtr<RHI::DescriptorTable>> descriptorTables;
			for (uint32_t i = 0; i < imageSpec.mips; i++)
			{
				descriptorTables.emplace_back(RHI::DescriptorTable::Create(tableInfo));
				descriptorTables.back()->SetImageView("u_input", environmentRaw->GetView(), 0);
				descriptorTables.back()->SetSamplerState("u_linearSampler", linearSampler->GetResource(), 0);
			}

			struct Constants
			{
				uint32_t mipIndex;
				uint32_t mipCount;
			} constants;

			for (uint32_t i = 0, size = CUBE_MAP_SIZE; i < imageSpec.mips; i++, size /= 2)
			{
				const uint32_t numGroups = glm::max(1u, Math::DivideRoundUp(size, 32u));

				constants.mipIndex = i;
				constants.mipCount = imageSpec.mips;

				descriptorTables[i]->SetImageView("o_output", environmentSpecular->GetArrayView(i), 0);

				commandBuffer->BindPipeline(pipeline);
				commandBuffer->BindDescriptorTable(descriptorTables[i]);
				commandBuffer->PushConstants(&constants, sizeof(Constants), 0);
				commandBuffer->Dispatch(numGroups, numGroups, 6);

				RHI::ResourceBarrierInfo imageBarrierInfo{};
				imageBarrierInfo.type = RHI::BarrierType::Image;
				imageBarrierInfo.imageBarrier().srcStage = RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
				imageBarrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::ShaderWrite;
				imageBarrierInfo.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				imageBarrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
				imageBarrierInfo.imageBarrier().subResource.levelCount = 1;
				imageBarrierInfo.imageBarrier().subResource.baseMipLevel = i;
				imageBarrierInfo.imageBarrier().resource = environmentSpecular;

				commandBuffer->ResourceBarrier({ imageBarrierInfo });
			}
		}

		// Diffuse
		{
			RHI::ImageSpecification imageSpec{};
			imageSpec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			imageSpec.width = DIFFUSE_MAP_SIZE;
			imageSpec.height = DIFFUSE_MAP_SIZE;
			imageSpec.usage = RHI::ImageUsage::Storage;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;
			imageSpec.mips = RHI::Utility::CalculateMipCount(DIFFUSE_MAP_SIZE, DIFFUSE_MAP_SIZE);
			imageSpec.debugName = "Environment - Diffuse";

			environmentDiffuse = RHI::Image::Create(imageSpec);
		
			{
				RHI::ResourceBarrierInfo barrierInfo{};
				barrierInfo.type = RHI::BarrierType::Image;
				barrierInfo.imageBarrier().srcStage = RHI::BarrierStage::None;
				barrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::None;
				barrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::Undefined;
				barrierInfo.imageBarrier().dstStage = RHI::BarrierStage::ComputeShader;
				barrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderWrite;
				barrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderWrite;
				barrierInfo.imageBarrier().resource = environmentDiffuse;
				commandBuffer->ResourceBarrier({ barrierInfo });
			}

			auto pipeline = ShaderMap::GetComputePipeline<IntegrateDiffuseCubeCS>(false);
			RHI::DescriptorTableCreateInfo tableInfo{};
			tableInfo.shader = pipeline->GetShader();

			RefPtr<RHI::DescriptorTable> descriptorTable = RHI::DescriptorTable::Create(tableInfo);
			descriptorTable->SetImageView("o_output", environmentDiffuse->GetArrayView(), 0);
			descriptorTable->SetImageView("u_input", environmentRaw->GetView(), 0);
			descriptorTable->SetSamplerState("u_linearSampler", linearSampler->GetResource(), 0);

			commandBuffer->BindPipeline(pipeline);
			commandBuffer->BindDescriptorTable(descriptorTable);

			const uint32_t groupCount = Math::DivideRoundUp(DIFFUSE_MAP_SIZE, CONVERSION_THREAD_GROUP_SIZE);
			commandBuffer->Dispatch(groupCount, groupCount, 6);

			{
				RHI::ResourceBarrierInfo imageBarrierInfo{};
				imageBarrierInfo.type = RHI::BarrierType::Image;
				imageBarrierInfo.imageBarrier().srcStage = RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().srcAccess = RHI::BarrierAccess::ShaderWrite;
				imageBarrierInfo.imageBarrier().srcLayout = RHI::ImageLayout::ShaderWrite;
				imageBarrierInfo.imageBarrier().dstStage = RHI::BarrierStage::PixelShader | RHI::BarrierStage::ComputeShader;
				imageBarrierInfo.imageBarrier().dstAccess = RHI::BarrierAccess::ShaderRead;
				imageBarrierInfo.imageBarrier().dstLayout = RHI::ImageLayout::ShaderRead;
				imageBarrierInfo.imageBarrier().resource = environmentDiffuse;

				commandBuffer->ResourceBarrier({ imageBarrierInfo });
			}

		}

		commandBuffer->End();
		commandBuffer->ExecuteAndWait();

		environmentDiffuse->GenerateMips();

		EnvironmentTextures result{};
		result.diffuse = environmentDiffuse;
		result.specular = environmentSpecular;

		return result;
	}

#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
	ShaderRuntimeValidator& Renderer::GetRuntimeShaderValidator()
	{
		return *s_instance->m_shaderValidator;
	}
#endif

	bool Renderer::OnEndOfFrameUpdate(AppPostFrameUpdateEvent& event)
	{
#ifdef VT_ENABLE_SHADER_RUNTIME_VALIDATION
		//s_rendererData->shaderValidator->ReadbackErrorBuffer();

		const auto& frameErrors = m_shaderValidator->GetValidationErrors();
		for (const auto& error : frameErrors)
		{
			VT_LOGC(Error, LogRender, error);
		}
#endif

		return false;
	}

	bool Renderer::OnPreRenderEvent(AppPreRenderEvent& event)
	{
		const uint32_t currentFrame = WindowManager::Get().GetMainWindow().GetSwapchain().GetCurrentFrame();

		m_deletionQueue.at(currentFrame).Flush();
		m_bindlessResourcesManager->Update();

		return false;
	}

	BindlessResourceRef<RHI::SamplerState> Renderer::GetSamplerInternal(const RHI::SamplerStateCreateInfo& samplerInfo)
	{
		const size_t hash = Utility::GetHashFromSamplerInfo(samplerInfo);
		if (m_samplers.contains(hash))
		{
			return m_samplers.at(hash);
		}

		BindlessResourceRef<RHI::SamplerState> samplerState = BindlessResource<RHI::SamplerState>::CreateRef(samplerInfo);
		m_samplers[hash] = samplerState;

		return samplerState;
	}

	void Renderer::CreateDefaultResources()
	{
		// Full white 1x1
		{
			constexpr uint32_t PIXEL_DATA = 0xffffffff;
			m_defaultResources.whiteTexture = Texture2D::Create(RHI::PixelFormat::R8G8B8A8_UNORM, 1, 1, &PIXEL_DATA);
			m_defaultResources.whiteTexture->handle = 0;
		}

		// Full black cube 1x1
		{
			constexpr uint32_t PIXEL_DATA[6] = { 0, 0, 0, 0, 0, 0 };

			RHI::ImageSpecification imageSpec{};
			imageSpec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
			imageSpec.usage = RHI::ImageUsage::Texture;
			imageSpec.width = 1;
			imageSpec.height = 1;
			imageSpec.layers = 6;
			imageSpec.isCubeMap = true;
			imageSpec.debugName = "BlackCube";

			m_defaultResources.blackCubeTexture = RHI::Image::Create(imageSpec, PIXEL_DATA);
		}

		// Full black 1x1x1
		{
			constexpr uint32_t PIXEL_DATA = 0xffffffff;
			
			RHI::ImageSpecification imageSpec{};
			imageSpec.format = RHI::PixelFormat::R8G8B8A8_UNORM;
			imageSpec.usage = RHI::ImageUsage::Texture;
			imageSpec.width = 1;
			imageSpec.height = 1;
			imageSpec.depth = 1;
			imageSpec.debugName = "Black1x1x1";
			imageSpec.imageType = RHI::ResourceType::Image3D;

			m_defaultResources.black1x1x1 = RHI::Image::Create(imageSpec, &PIXEL_DATA);
		}

		GenerateDFGLuT();

		// Default material
		{
			m_defaultResources.defaultMaterial = CreateRef<RenderMaterial>("DefaultMaterial", ShaderMap::Get<OpaqueDefaultMaterialCS>());
		}

		// Default mesh
		{
			m_defaultResources.defaultMesh = ShapeLibrary::GetCube();
		}
	}

	void Renderer::GenerateDFGLuT()
	{
		constexpr uint32_t DFGSize = 512;

		RHI::ImageSpecification spec{};
		spec.format = RHI::PixelFormat::R16G16B16A16_SFLOAT;
		spec.usage = RHI::ImageUsage::AttachmentStorage;
		spec.width = DFGSize;
		spec.height = DFGSize;
		spec.debugName = "DFGLuT";

		m_defaultResources.DFGLuT = RHI::Image::Create(spec);

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();

		RenderGraph renderGraph{ commandBuffer };
		RenderGraphImageHandle targetImageHandle = renderGraph.AddExternalImage(m_defaultResources.DFGLuT);

		renderGraph.AddPass("Pre integrate DFG Pass",
		[&](RenderGraph::Builder& builder)
		{
			builder.WriteResource(targetImageHandle);
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			RenderingInfo renderingInfo = context.CreateRenderingInfo(DFGSize, DFGSize, { targetImageHandle });

			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shader = ShaderMap::Get<GeneratePreIntegratedDFG>();
			pipelineInfo.cullMode = RHI::CullMode::None;

			auto pipeline = ShaderMap::GetRenderPipeline(pipelineInfo);

			context.BeginRendering(renderingInfo);
			RCUtils::DrawFullscreenTriangle(context, pipeline);
			context.EndRendering();
		});

		renderGraph.Compile();
		renderGraph.ExecuteImmediateAndWait();
	}

	static Vector<std::filesystem::path> FindShaderIncludes(const std::filesystem::path& filePath)
	{
		constexpr const char* INCLUDE_KEYWORD = "#include";

		std::ifstream input{ filePath };
		if (!input.is_open())
		{
			return {};
		}

		std::string shaderString{};
		input.seekg(0, std::ios::end);
		shaderString.resize(input.tellg());
		input.seekg(0, std::ios::beg);
		input.read(&shaderString[0], shaderString.size());

		input.close();

		Vector<std::filesystem::path> resultIncludes{};

		size_t offset = shaderString.find(INCLUDE_KEYWORD, 0);
		while (offset != std::string::npos)
		{
			size_t openOffset = shaderString.find_first_of("\"<", offset);
			if (openOffset == std::string::npos)
			{
				break;
			}

			size_t closeOffset = shaderString.find_first_of("\">", openOffset + 1);
			if (closeOffset == std::string::npos)
			{
				break;
			}

			std::string includeString = shaderString.substr(openOffset + 1, closeOffset - openOffset - 1);

			// Find real path
			if (std::filesystem::exists(filePath.parent_path() / includeString))
			{
				resultIncludes.emplace_back(filePath.parent_path() / includeString);
			}
			else if (std::filesystem::exists(ProjectManager::GetEngineShaderIncludeDirectory() / includeString))
			{
				resultIncludes.emplace_back(ProjectManager::GetEngineShaderIncludeDirectory() / includeString);
			}
			else if (std::filesystem::exists(ProjectManager::GetAssetsDirectory() / includeString))
			{
				resultIncludes.emplace_back(ProjectManager::GetAssetsDirectory() / includeString);
			}

			offset = shaderString.find(INCLUDE_KEYWORD, offset + 1);
		}

		return resultIncludes;
	}
}
