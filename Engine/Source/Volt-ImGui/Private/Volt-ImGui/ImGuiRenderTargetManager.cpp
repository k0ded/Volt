#include "Volt-ImGui/ImGuiRenderTargetManager.h"

#include <RHIModule/Graphics/Swapchain.h>

#include <WindowModule/Window.h>

namespace Volt
{
	RefPtr<RHI::Image> ImGuiRenderTargetManager::GetRenderTargetForWindow(Window* window, uint32_t desiredWidth, uint32_t desiredHeight)
	{
		if (m_renderTargets.contains(window))
		{
			RenderTarget& renderTarget = m_renderTargets.at(window);
			const bool requiresResize = desiredWidth != renderTarget.image->GetWidth() || desiredHeight != renderTarget.image->GetHeight();

			if (requiresResize)
			{
				renderTarget.image = CreateRenderTargetForWindow(window, desiredWidth, desiredHeight);
			}

			return renderTarget.image;
		}

		RenderTarget& renderTarget = m_renderTargets[window];
		renderTarget.image = CreateRenderTargetForWindow(window, desiredWidth, desiredHeight);

		return renderTarget.image;
	}

	void ImGuiRenderTargetManager::RemoveWindowRenderTarget(Window* window)
	{
		if (m_renderTargets.contains(window))
		{
			m_renderTargets.erase(window);
		}
	}

	RefPtr<RHI::Image> ImGuiRenderTargetManager::CreateRenderTargetForWindow(Window* window, uint32_t desiredWidth, uint32_t desiredHeight)
	{
		RHI::ImageDesc desc{};
		desc.format = RHI::PixelFormat::R8G8B8A8_UNORM;
		desc.width = desiredWidth;
		desc.height = desiredHeight;
		desc.imageType = RHI::ResourceType::Image2D;
		desc.usage = RHI::ImageUsage::AttachmentStorage;
		desc.generateMips = false;
		desc.initializeImage = false;
		desc.debugName = "ImGui.RenderTarget";

		return RHI::Image::Create(desc);
	}
}
