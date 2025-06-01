#include "vrpch.h"
#include "Volt-Renderer/BlueNoise.h"

#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/Renderer.h"

#include <RenderCore/RenderGraph2/RenderGraph2.h>

#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Math/Math.h>

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

	void BlueNoise::Setup(BlueNoiseShaderParameters& parameters, const BlueNoiseTextures& blueNoiseTextures)
	{
		parameters.blueNoiseScalarTexture = blueNoiseTextures.blueNoiseScalarTexture;
		parameters.blueNoiseVec2Texture = blueNoiseTextures.blueNoiseVec2Texture;
		parameters.blueNoiseRGBATexture = blueNoiseTextures.blueNoiseRGBATexture;
		//parameters.pointWrapSampler = Renderer::GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>()->GetResourceHandle();
		parameters.moduloMasks = s_blueNoiseData.moduloMasks;
		parameters.dimensions = s_blueNoiseData.dimensions;
	}

	BlueNoiseTextures BlueNoise::GetBlueNoiseTextures(RenderGraph2& renderGraph)
	{
		BlueNoiseTextures result;
		//result.blueNoiseRGBATexture = renderGraph.RegisterExternalTexture(s_blueNoiseData.rgbaBlueNoise->GetImage());
		//result.blueNoiseVec2Texture = renderGraph.RegisterExternalTexture(s_blueNoiseData.vec2BlueNoise->GetImage());
		//result.blueNoiseScalarTexture = renderGraph.RegisterExternalTexture(s_blueNoiseData.scalarBlueNoise->GetImage());

		return result;
	}

	void BlueNoise::LoadBlueNoiseTextures()
	{
		// Spatiotemporal
		s_blueNoiseData.scalarBlueNoise = AssetManager::GetAsset<Texture2D>("Engine/Textures/STBlueNoise_scalar_128x128x64.vtasset");
		s_blueNoiseData.vec2BlueNoise = AssetManager::GetAsset<Texture2D>("Engine/Textures/STBlueNoise_vec2_128x128x64.vtasset");

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
