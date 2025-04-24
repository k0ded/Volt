#pragma once

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	class Texture2D;

	struct BlueNoiseData
	{
		Ref<Texture2D> scalarBlueNoise;
		Ref<Texture2D> rgbaBlueNoise;
		Ref<Texture2D> vec2BlueNoise;
		glm::uvec3 moduloMasks;
		glm::uvec3 dimensions;
	};

	BEGIN_SHADER_PARAMETER_STRUCT(BlueNoiseShaderParameters)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float>, blueNoiseScalarTexture)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, blueNoiseVec2Texture)
		SHADER_PARAMETER_IMAGE(vt::Tex2D<float4>, blueNoiseRGBATexture)
		SHADER_PARAMETER_SAMPLER(vt::TextureSampler, pointWrapSampler)
		SHADER_PARAMETER(uint3, moduloMasks)
		SHADER_PARAMETER(uint3, dimensions)
	END_SHADER_PARAMETER_STRUCT()

	struct BlueNoiseTextures
	{
		RenderGraphImageHandle blueNoiseScalarTexture;
		RenderGraphImageHandle blueNoiseVec2Texture;
		RenderGraphImageHandle blueNoiseRGBATexture;
	};

	class BlueNoise
	{
	public:
		BlueNoise();
		~BlueNoise();

		static void Build(RenderGraph::Builder& builder, const BlueNoiseTextures& blueNoiseTextures);
		static void Setup(BlueNoiseShaderParameters& parameters, const BlueNoiseTextures& blueNoiseTextures);
		
		static BlueNoiseTextures GetBlueNoiseTextures(RenderGraph& renderGraph);

	private:
		void LoadBlueNoiseTextures();
	};
}
