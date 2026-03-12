#pragma once

#include "Volt-Renderer/Texture/Texture2D.h"

#include <RenderCore/RenderGraph/ShaderParameterStruct.h>

#include <AssetSystem/AssetReference.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class RenderGraph;

	struct BlueNoiseData
	{
		AssetReference<Texture2D> scalarBlueNoise;
		AssetReference<Texture2D> rgbaBlueNoise;
		AssetReference<Texture2D> vec2BlueNoise;
		glm::uvec3 moduloMasks;
		glm::uvec3 dimensions;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, BlueNoiseScalarTexture)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, BlueNoiseVec2Texture)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, BlueNoiseRGBATexture)
		SHADER_PARAMETER_SAMPLER(BlueNoiseSampler)
		SHADER_PARAMETER(uint3, BlueNoiseModuloMasks)
		SHADER_PARAMETER(uint3, BlueNoiseDimensions)
	END_SHADER_PARAMETER_STRUCT()

	struct BlueNoiseTextures
	{
		RGTextureSRVRef blueNoiseScalarTexture;
		RGTextureSRVRef blueNoiseVec2Texture;
		RGTextureSRVRef blueNoiseRGBATexture;
	};

	class BlueNoise
	{
	public:
		BlueNoise();
		~BlueNoise();

		static BlueNoiseShaderParameters GetBlueNoiseParameters(RenderGraph& renderGraph);

	private:
		void LoadBlueNoiseTextures();
	};
}
