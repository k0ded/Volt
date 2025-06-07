#include "vrpch.h"
#include "Volt-Renderer/Renderer.h"

#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/RenderMaterial.h"
#include "Volt-Renderer/ShapeLibrary.h"

#include <Volt-Core/Project/ProjectManager.h>

#include <AssetSystem/AssetManager.h>
#include <JobSystem/TaskGraph.h>

#include <RenderCore/RenderGraph/RenderGraphExecutionThread.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Resources/BindlessResourcesManager.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>

#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Descriptors/DescriptorTable.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/RHIFeatures.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(Renderer, Engine, 3);

	// #TODO_Ivar: Convert to render graph
#if 0
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
	REGISTER_SHADER(IntegrateDiffuseCubeCS);
#endif

	struct EquirectangularToCubemapCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(EquirectangularToCubemapCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float3>, RWOutput)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, EquirectangularMap)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(EquirectangularToCubemapCS, "Engine/Shaders/Source/Environment/EquirectangularToCubemap.hlsl", "MainCS", Compute);

	struct IntegrateSpecularCubeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(IntegrateSpecularCubeCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float3>, RWOutput)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, Input)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
			SHADER_PARAMETER(uint, MipIndex)
			SHADER_PARAMETER(uint, MipCount)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(IntegrateSpecularCubeCS, "Engine/Shaders/Source/PBR/IntegrateSpecularCube.hlsl", "MainCS", Compute);

	struct IntegrateDiffuseCubeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(IntegrateDiffuseCubeCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float3>, RWOutput)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, Input)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(IntegrateDiffuseCubeCS, "Engine/Shaders/Source/PBR/IntegrateDiffuseCube.hlsl", "MainCS", Compute);

	struct GeneratePreIntegratedDFGPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GeneratePreIntegratedDFGPS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(GeneratePreIntegratedDFGPS, "Engine/Shaders/Source/PBR/GeneratePreIntegratedDFG.hlsl", "MainPS", Pixel);

	namespace Utility
	{
		inline static const size_t GetHashFromSamplerDesc(const RHI::SamplerStateDesc& info)
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
		// Bindless resources manager
		if (RHI::RHICanUseBindless())
		{
			m_bindlessResourcesManager = CreateScope<BindlessResourcesManager>();
		}

		m_descriptorTableCache = CreateScope<DescriptorTableCache>();
		m_samplerStateCache = CreateScope<SamplerStateCache>();

		RenderGraphExecutionThread::Initialize(RenderGraphExecutionThread::ExecutionMode::Multithreaded);

		CreateDefaultResources();
		m_blueNoise = CreateScope<BlueNoise>();
	}

	void Renderer::Shutdown()
	{
		RenderGraphExecutionThread::Shutdown();

		m_blueNoise.reset();

		m_defaultResources.Clear();
		m_samplers.clear();

		ShapeLibrary::Shutdown();

		m_shaderMap = nullptr;
		m_samplerStateCache = nullptr;
		m_descriptorTableCache = nullptr;
		m_bindlessResourcesManager = nullptr;
	}

	const uint32_t Renderer::GetFramesInFlight()
	{
		return RHI::Swapchain::FramesInFlight;
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

		constexpr uint32_t CubeMapSize = 1024;
		constexpr uint32_t DiffuseMapSize = 256;
		constexpr uint32_t ConversionThreadGroupSize = 32;

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();
		RenderGraph renderGraph{ commandBuffer };

		RGTextureRef environmentRaw = renderGraph.CreateTexture(RGTextureDesc::CreateCube<RHI::PixelFormat::B10G11R11_UFLOAT_PACK32>(CubeMapSize, CubeMapSize, RHI::ImageUsage::Storage, "EquirectangularTexture"));

		// Convert to equirectangular
		{
			EquirectangularToCubemapCS::Parameters* passParameters = renderGraph.AllocParameters<EquirectangularToCubemapCS::Parameters>();
			passParameters->RWOutput = renderGraph.CreateUAV(environmentRaw);
			passParameters->EquirectangularMap = renderGraph.CreateSRV(renderGraph.RegisterExternalTexture(environmentTexture->GetImage()));
			passParameters->LinearSampler = SamplerStateCache::GetTrilinearSampler();

			const uint32_t groupCount = Math::DivideRoundUp(CubeMapSize, ConversionThreadGroupSize);

			auto shader = ShaderMap::Get<EquirectangularToCubemapCS>();
			ComputeShaderUtils::AddPass<EquirectangularToCubemapCS>(renderGraph,
				"Convert to Equirectangular",
				shader,
				passParameters,
				{ groupCount, groupCount, 6 });
		}

		// Specular
		RGTextureDesc specularDesc{};
		specularDesc.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
		specularDesc.width = CubeMapSize;
		specularDesc.height = CubeMapSize;
		specularDesc.usage = RHI::ImageUsage::Storage;
		specularDesc.layers = 6;
		specularDesc.isCubeMap = true;
		specularDesc.mips = RHI::Utility::CalculateMipCount(CubeMapSize, CubeMapSize);
		specularDesc.debugName = "Environment - Specular";
		
		RGTextureRef environmentSpecular = renderGraph.CreateTexture(specularDesc);
		{
			for (uint32_t i = 0, size = CubeMapSize; i < specularDesc.mips; ++i, size /= 2)
			{
				IntegrateSpecularCubeCS::Parameters* passParameters = renderGraph.AllocParameters<IntegrateSpecularCubeCS::Parameters>();
				
				RGTextureUAVDesc uavDesc{};
				uavDesc.textureResource = environmentSpecular;
				uavDesc.baseMipLevel = i;
				uavDesc.mipCount = 1;

				passParameters->RWOutput = renderGraph.CreateUAV(uavDesc);
				passParameters->Input = renderGraph.CreateSRV(environmentRaw);
				passParameters->LinearSampler = SamplerStateCache::GetTrilinearSampler();
				passParameters->MipIndex = i;
				passParameters->MipCount = specularDesc.mips;

				const uint32_t numGroups = glm::max(1u, Math::DivideRoundUp(size, 32u));

				auto shader = ShaderMap::Get<IntegrateSpecularCubeCS>();
				ComputeShaderUtils::AddPass<IntegrateSpecularCubeCS>(renderGraph,
					"Integrate Specular",
					shader,
					passParameters,
					{ numGroups, numGroups, 6 });
			}
		}

		// Diffuse
		RGTextureDesc diffuseDesc{};
		diffuseDesc.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;
		diffuseDesc.width = DiffuseMapSize;
		diffuseDesc.height = DiffuseMapSize;
		diffuseDesc.usage = RHI::ImageUsage::Storage;
		diffuseDesc.layers = 6;
		diffuseDesc.isCubeMap = true;
		diffuseDesc.mips = RHI::Utility::CalculateMipCount(DiffuseMapSize, DiffuseMapSize);
		diffuseDesc.debugName = "Environment - Diffuse";

		RGTextureRef environmentDiffuse = renderGraph.CreateTexture(diffuseDesc);
		{
			IntegrateDiffuseCubeCS::Parameters* passParameters = renderGraph.AllocParameters<IntegrateDiffuseCubeCS::Parameters>();
			passParameters->RWOutput = renderGraph.CreateUAV(environmentDiffuse);
			passParameters->Input = renderGraph.CreateSRV(environmentRaw);
			passParameters->LinearSampler = SamplerStateCache::GetTrilinearSampler();

			const uint32_t groupCount = Math::DivideRoundUp(DiffuseMapSize, ConversionThreadGroupSize);

			auto shader = ShaderMap::Get<IntegrateDiffuseCubeCS>();
			ComputeShaderUtils::AddPass<IntegrateDiffuseCubeCS>(renderGraph,
				"Integrate Diffuse",
				shader,
				passParameters,
				{ groupCount, groupCount, 6 });
		}

		EnvironmentTextures result{};

		renderGraph.EnqueueTextureExtraction(environmentSpecular, &result.specular);
		renderGraph.EnqueueTextureExtraction(environmentDiffuse, &result.diffuse);

		renderGraph.Compile();
		renderGraph.ExecuteImmediateAndWait();

		result.diffuse->GenerateMips();

		return result;
	}

	bool Renderer::OnEndOfFrameUpdate(AppPostFrameUpdateEvent& event)
	{
		m_frameIndex++;
		return false;
	}

	bool Renderer::OnPreRenderEvent(AppPreRenderEvent& event)
	{
		if (RHI::RHICanUseBindless())
		{
			m_bindlessResourcesManager->Update();
		}

		m_descriptorTableCache->Update();

		return false;
	}

	BindlessResourceRef<RHI::SamplerState> Renderer::GetSamplerInternal(const RHI::SamplerStateDesc& samplerInfo)
	{
		const size_t hash = Utility::GetHashFromSamplerDesc(samplerInfo);
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

			RHI::ImageDesc imageSpec{};
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
			
			RHI::ImageDesc imageSpec{};
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
#if 0
			m_defaultResources.defaultMaterial = CreateRef<RenderMaterial>("DefaultMaterial", ShaderMap::Get<OpaqueDefaultMaterialCS>());
#endif
		}

		// Default mesh
		{
			m_defaultResources.defaultMesh = ShapeLibrary::GetCube();
		}
	}

	void Renderer::GenerateDFGLuT()
	{
		constexpr uint32_t DFGSize = 512;

		RHI::ImageDesc spec{};
		spec.format = RHI::PixelFormat::R16G16B16A16_SFLOAT;
		spec.usage = RHI::ImageUsage::AttachmentStorage;
		spec.width = DFGSize;
		spec.height = DFGSize;
		spec.debugName = "DFGLuT";

		m_defaultResources.DFGLuT = RHI::Image::Create(spec);

		RefPtr<RHI::CommandBuffer> commandBuffer = RHI::CommandBuffer::Create();

		RenderGraph renderGraph{ commandBuffer };

		GeneratePreIntegratedDFGPS::Parameters* passParameters = renderGraph.AllocParameters<GeneratePreIntegratedDFGPS::Parameters>();
		passParameters->renderTargets.renderTargets[0] = renderGraph.RegisterExternalTexture(m_defaultResources.DFGLuT);

		RefPtr<RHI::Shader> vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		RefPtr<RHI::Shader> pixelShader = ShaderMap::Get<GeneratePreIntegratedDFGPS>();

		renderGraph.AddPass("Pre integrate DFG Pass",
			RenderGraphPassFlags::None,
			passParameters,
			[passParameters, vertexShader, pixelShader](RenderContext& context) 
		{
			RHI::RenderPipelineCreateInfo pipelineInfo{};
			pipelineInfo.shaders = { vertexShader, pixelShader };
			pipelineInfo.cullMode = RHI::CullMode::None;

			auto pipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

			RenderingInfo renderingInfo = context.CreateRenderingInfo(DFGSize, DFGSize, passParameters->renderTargets);
			context.BeginRendering(renderingInfo);
			context.BindPipeline(pipeline);
			context.Draw(3, 1, 0, 0);
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
