#pragma once

#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class Texture2D;
	class RenderGraph;

	struct BlueNoiseData
	{
		Ref<Texture2D> scalarBlueNoise;
		Ref<Texture2D> rgbaBlueNoise;
		Ref<Texture2D> vec2BlueNoise;
		glm::uvec3 moduloMasks;
		glm::uvec3 dimensions;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float>, blueNoiseScalarTexture)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, blueNoiseVec2Texture)
		SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, blueNoiseRGBATexture)
		//SHADER_PARAMETER_SAMPLER(vt::TextureSampler, pointWrapSampler) // #TODO_Ivar: Implement samplers.
		SHADER_PARAMETER(uint3, moduloMasks)
		SHADER_PARAMETER(uint3, dimensions)
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

		static void Setup(BlueNoiseShaderParameters& parameters, const BlueNoiseTextures& blueNoiseTextures);
		
		static BlueNoiseTextures GetBlueNoiseTextures(RenderGraph& renderGraph);

	private:
		void LoadBlueNoiseTextures();
	};
}
