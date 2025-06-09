#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	class Camera;

	struct RenderView
	{
		uint32_t width;
		uint32_t height;
		uint32_t frameIndex;

		Ref<Camera> camera;

		RGUniformBufferRef viewUniformBuffer;
	};
}
