#pragma once

#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/Resources/RenderGraphTexture.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;

	struct SystemTextures
	{
		RGTextureRef white1x1;
		RGTextureRef black1x1;
		RGTextureRef black1x1x1;
		RGTextureRef blackCubeTexture;

		VTR_API static SystemTextures& SetupSystemTextures(RenderGraph& renderGraph, RenderGraphBlackboard& renderGraphBlackboard);
	};
}
