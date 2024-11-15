#pragma once

#include <RenderCore/RenderGraph/RenderGraph.h>
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
		glm::uvec3 moduloMasks;
		glm::uvec3 dimensions;
	};

	struct BlueNoiseTextures
	{
		RenderGraphImageHandle blueNoiseScalarTexture;
		RenderGraphImageHandle blueNoiseRGBATexture;
	};

	class BlueNoise
	{
	public:
		BlueNoise();
		~BlueNoise();

		static void Build(RenderGraph::Builder& builder, const BlueNoiseTextures& blueNoiseTextures);
		static void Setup(RenderContext& renderContext, const BlueNoiseTextures& blueNoiseTextures);
		
		static BlueNoiseTextures GetBlueNoiseTextures(RenderGraph& renderGraph);

	private:
		void LoadBlueNoiseTextures();
	};
}
