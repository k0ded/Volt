#include "vrpch.h"

#include "Volt-Renderer/SystemTextures.h"
#include "Volt-Renderer/RendererUtilities.h"

#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraph.h>

namespace Volt
{
	SystemTextures& SystemTextures::SetupSystemTextures(RenderGraph& renderGraph, RenderGraphBlackboard& renderGraphBlackboard)
	{
		SystemTextures& systemTextures = renderGraphBlackboard.Add<SystemTextures>();
		systemTextures.white1x1 = renderGraph.RegisterExternalTexture(RendererUtilities::GetDefaultResources().white1x1);
		systemTextures.black1x1 = renderGraph.RegisterExternalTexture(RendererUtilities::GetDefaultResources().black1x1);
		systemTextures.black1x1x1 = renderGraph.RegisterExternalTexture(RendererUtilities::GetDefaultResources().black1x1x1);
		systemTextures.blackCubeTexture = renderGraph.RegisterExternalTexture(RendererUtilities::GetDefaultResources().blackCubeTexture);

		return systemTextures;
	}
}
