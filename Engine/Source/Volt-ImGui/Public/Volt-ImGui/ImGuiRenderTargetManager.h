#pragma once

#include "Volt-ImGui/Config.h"

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	class Window;

	class VTIMGUI_API ImGuiRenderTargetManager
	{
	public:
		struct RenderTarget
		{
			IntRef<RHI::Image> image;
		};

		IntRef<RHI::Image> GetRenderTargetForWindow(Window* window, uint32_t desiredWidth, uint32_t desiredHeight);
		void RemoveWindowRenderTarget(Window* window);

		const Map<Window*, ImGuiRenderTargetManager::RenderTarget>& GetAllRenderTargets() const { return m_renderTargets; }

	private:
		IntRef<RHI::Image> CreateRenderTargetForWindow(Window* window, uint32_t desiredWidth, uint32_t desiredHeight);
		Map<Window*, RenderTarget> m_renderTargets;
	};
}
