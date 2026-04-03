#include "vrpch.h"
#include "Volt-Renderer/RendererUtilities.h"

#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/ShapeLibrary.h"

#include <AssetSystem/AssetManager.h>

#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderSubSystem.h>
#include <RenderCore/SamplerStateCache.h>

#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/RHIFeatures.h>
#include <RHIModule/RHIModule.h>

#include <AssetSystem/AssetManagerSubSystem.h>

#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Math/Math.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(RendererUtilities, Minimal, Engine);

	struct EquirectangularToCubemapCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(EquirectangularToCubemapCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float3>, RWOutput)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, EquirectangularMap)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(EquirectangularToCubemapCS, "Engine/Shaders/Source/Environment/EquirectangularToCubemap.hlsl", "MainCS", Compute);

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
	VT_REGISTER_SHADER(IntegrateSpecularCubeCS, "Engine/Shaders/Source/PBR/IntegrateSpecularCube.hlsl", "MainCS", Compute);

	struct IntegrateDiffuseCubeCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(IntegrateDiffuseCubeCS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_UAV(RWTexture2DArray<float3>, RWOutput)
			SHADER_PARAMETER_TEXTURE_SRV(TextureCube<float3>, Input)
			SHADER_PARAMETER_SAMPLER(LinearSampler)
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(IntegrateDiffuseCubeCS, "Engine/Shaders/Source/PBR/IntegrateDiffuseCube.hlsl", "MainCS", Compute);

	struct GeneratePreIntegratedDFGPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GeneratePreIntegratedDFGPS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(GeneratePreIntegratedDFGPS, "Engine/Shaders/Source/PBR/GeneratePreIntegratedDFG.hlsl", "MainPS", Pixel);

	struct GeneratePreIntegratedBRDFPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(GeneratePreIntegratedBRDFPS)

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
	VT_REGISTER_SHADER(GeneratePreIntegratedBRDFPS, "Engine/Shaders/Source/PBR/GenerateBRDF.hlsl", "MainPS", Pixel);

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

	RendererUtilities::RendererUtilities()
	{
		VT_ASSERT(!s_instance);
		s_instance = this;
	}

	RendererUtilities::~RendererUtilities()
	{
		s_instance = nullptr;
	}

	void RendererUtilities::Initialize()
	{
		CreateDefaultResources();
		CreateBlueNoise();
	}

	void RendererUtilities::CreateBlueNoise()
	{
		m_blueNoise = CreateUnique<BlueNoise>();
	}

	void RendererUtilities::Shutdown()
	{
		m_blueNoise.Reset();

		m_defaultResources.Clear();

		ShapeLibrary::Shutdown();
	}

	const uint32_t RendererUtilities::GetFramesInFlight()
	{
		return RHI::RHICapabilities::NumFramesInFlight;
	}

	const DefaultResources& RendererUtilities::GetDefaultResources()
	{
		return s_instance->m_defaultResources;
	}

	RendererUtilities::EnvironmentTextures RendererUtilities::GenerateEnvironmentTextures(AssetHandle baseTextureHandle)
	{
		AssetReference<Texture2D> environmentTexture = g_assetManager->GetAssetImmediately<Texture2D>(baseTextureHandle);

		// Texture doesn't exist.
		if (!environmentTexture)
		{
			return {};
		}

		IntRef<RHI::Image> environmentTextureImage;

		// Get the image from the environment texture.
		{
			if (!environmentTexture->IsValid())
			{
				return {};
			}

			environmentTextureImage = environmentTexture->GetImage();
		}

		constexpr uint32_t CubeMapSize = 1024;
		constexpr uint32_t DiffuseMapSize = 32;
		constexpr uint32_t ConversionThreadGroupSize = 32;

		RenderGraph renderGraph{};

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

		return result;
	}

	void RendererUtilities::CreateDefaultResources()
	{
		// Full white 1x1
		{
			constexpr uint32_t PIXEL_DATA = 0xffffffff;

			RHI::ImageDesc imageSpec{};
			imageSpec.format = RHI::PixelFormat::R8G8B8A8_UNORM;
			imageSpec.usage = RHI::ImageUsage::Texture;
			imageSpec.width = 1;
			imageSpec.height = 1;
			imageSpec.debugName = "White1x1";

			m_defaultResources.white1x1 = RHI::Image::Create(imageSpec, &PIXEL_DATA);
		}

		// Full black 1x1
		{
			constexpr uint32_t PIXEL_DATA = 0x000000ff;

			RHI::ImageDesc imageSpec{};
			imageSpec.format = RHI::PixelFormat::R8G8B8A8_UNORM;
			imageSpec.usage = RHI::ImageUsage::Texture;
			imageSpec.width = 1;
			imageSpec.height = 1;
			imageSpec.debugName = "Blax1x1";

			m_defaultResources.black1x1 = RHI::Image::Create(imageSpec, &PIXEL_DATA);
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
			m_defaultResources.defaultMaterial = CreateRef<RenderMaterial>("DefaultMaterial");

			m_defaultResources.defaultTranslucentMaterial = CreateRef<RenderMaterial>("DefaultTranslucentMaterial");
			m_defaultResources.defaultTranslucentMaterial->SetMaterialBlendMode(MaterialBlendMode::Translucent);
		}

		// Default mesh
		{
			m_defaultResources.defaultMesh = ShapeLibrary::GetCube();
		}

		// Generate mip maps shader
		{
			RHI::ShaderCreateInfo shaderCreateInfo{};
			shaderCreateInfo.entryPoint = "GenerateMipMapsCS";
			shaderCreateInfo.name = "GenerateMipMapsCS";
			shaderCreateInfo.sourceFilepath = "Engine/Shaders/Source/Utility/GenerateMipMapsCS.hlsl";
			shaderCreateInfo.stage = RHI::ShaderStage::Compute;

			m_defaultResources.generateMipMapsShader = RHI::Shader::Create(shaderCreateInfo);
		}
	}

	void RendererUtilities::GenerateDFGLuT()
	{
		constexpr uint32_t DFGSize = 512;

		RHI::ImageDesc spec{};
		spec.format = RHI::PixelFormat::R16G16_SFLOAT;
		spec.usage = RHI::ImageUsage::AttachmentStorage;
		spec.width = DFGSize;
		spec.height = DFGSize;
		spec.debugName = "DFGLuT";

		m_defaultResources.DFGLuT = RHI::Image::Create(spec);

		RenderGraph renderGraph{};

		GeneratePreIntegratedBRDFPS::Parameters* passParameters = renderGraph.AllocParameters<GeneratePreIntegratedBRDFPS::Parameters>();
		passParameters->renderTargets.renderTargets[0] = renderGraph.RegisterExternalTexture(m_defaultResources.DFGLuT);

		IntRef<RHI::Shader> vertexShader = ShaderMap::Get<FullscreenTriangleVS>();
		IntRef<RHI::Shader> pixelShader = ShaderMap::Get<GeneratePreIntegratedBRDFPS>();

		renderGraph.AddPass("Pre integrate DFG Pass",
			RenderGraphPassFlags::Raster,
			passParameters,
			[passParameters, vertexShader, pixelShader](RenderContext& context) 
		{
			GraphicsPipelineState pipelineState{};
			pipelineState.shaders = { vertexShader, pixelShader };
			pipelineState.cullMode = RHI::CullMode::None;
			pipelineState.renderTargets = passParameters->renderTargets;

			RenderingInfo renderingInfo = context.CreateRenderingInfo(DFGSize, DFGSize, passParameters->renderTargets);
			context.BeginRendering(renderingInfo);
			context.SetPipelineState(pipelineState);
			context.Draw(3, 1, 0, 0);
			context.EndRendering();
		});

		renderGraph.Compile();
		renderGraph.ExecuteImmediateAndWait();
	}

	void RendererUtilities::GetSubSystemDependencies(SubSystemDependencyList& outDependencies)
	{
		outDependencies.AddDependency<ShaderSubSystem>();
		outDependencies.AddDependency<AssetManagerSubSystem>();
	}
}
