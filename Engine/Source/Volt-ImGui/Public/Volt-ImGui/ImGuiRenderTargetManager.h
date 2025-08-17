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
			RefPtr<RHI::Image> image;
		};

		RefPtr<RHI::Image> GetRenderTargetForWindow(Window* window);
		void RemoveWindowRenderTarget(Window* window);

		const Map<Window*, ImGuiRenderTargetManager::RenderTarget>& GetAllRenderTargets() const { return m_renderTargets; }

	private:
		RefPtr<RHI::Image> CreateRenderTargetForWindow(Window* window);
		Map<Window*, RenderTarget> m_renderTargets;
	};
}
