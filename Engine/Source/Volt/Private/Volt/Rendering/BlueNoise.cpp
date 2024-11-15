#include "vtpch.h"
#include "Volt/Rendering/BlueNoise.h"

#include "Volt/Rendering/Texture/Texture2D.h"
#include "Volt/Rendering/Renderer.h"
#include "Volt/Math/Math.h"

#include <RenderCore/RenderGraph/RenderContext.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	static BlueNoiseData s_blueNoiseData;

	BlueNoise::BlueNoise()
	{
		LoadBlueNoiseTextures();
	}

	BlueNoise::~BlueNoise()
	{
		s_blueNoiseData = {};
	}

	void BlueNoise::Build(RenderGraph::Builder& builder, const BlueNoiseTextures& blueNoiseTextures)
	{
		builder.ReadResource(blueNoiseTextures.blueNoiseScalarTexture);
		builder.ReadResource(blueNoiseTextures.blueNoiseRGBATexture);
	}

	void BlueNoise::Setup(RenderContext& renderContext, const BlueNoiseTextures& blueNoiseTextures)
	{
		renderContext.SetConstant("blueNoiseData.blueNoiseScalarTexture"_sh, blueNoiseTextures.blueNoiseScalarTexture);
		renderContext.SetConstant("blueNoiseData.blueNoiseRGBATexture"_sh, blueNoiseTextures.blueNoiseRGBATexture);
		renderContext.SetConstant("blueNoiseData.pointWrapSampler"_sh, Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle());
		renderContext.SetConstant("blueNoiseData.moduloMasks"_sh, s_blueNoiseData.moduloMasks);
		renderContext.SetConstant("blueNoiseData.dimensions"_sh, s_blueNoiseData.dimensions);
	}

	BlueNoiseTextures BlueNoise::GetBlueNoiseTextures(RenderGraph& renderGraph)
	{
		BlueNoiseTextures result;
		result.blueNoiseRGBATexture = renderGraph.AddExternalImage(s_blueNoiseData.rgbaBlueNoise->GetImage());
		result.blueNoiseScalarTexture = renderGraph.AddExternalImage(s_blueNoiseData.scalarBlueNoise->GetImage());

		return result;
	}

	void BlueNoise::LoadBlueNoiseTextures()
	{
		// Spatiotemporal
		s_blueNoiseData.scalarBlueNoise = AssetManager::GetAsset<Texture2D>("Engine/Textures/STBlueNoise_scalar_128x128x64.vtasset");

		const uint32_t width = s_blueNoiseData.scalarBlueNoise->GetWidth();
		const uint32_t height = s_blueNoiseData.scalarBlueNoise->GetHeight();

		s_blueNoiseData.dimensions = glm::uvec3(width, width, height / glm::max(1u, width));
		s_blueNoiseData.moduloMasks = glm::uvec3(
			(1u << Math::FloorLog2(s_blueNoiseData.dimensions.x)) - 1,
			(1u << Math::FloorLog2(s_blueNoiseData.dimensions.y)) - 1,
			(1u << Math::FloorLog2(s_blueNoiseData.dimensions.z)) - 1);

		// RGBA
		s_blueNoiseData.rgbaBlueNoise = AssetManager::GetAsset<Texture2D>("Engine/Textures/BlueNoise_rgba_512x512.vtasset");
	}
}
