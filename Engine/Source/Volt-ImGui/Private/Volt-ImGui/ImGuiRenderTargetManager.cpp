#include "Volt-ImGui/ImGuiRenderTargetManager.h"

#include <RHIModule/Graphics/Swapchain.h>

#include <WindowModule/Window.h>

namespace Volt
{
	RefPtr<RHI::Image> ImGuiRenderTargetManager::GetRenderTargetForWindow(Window* window)
	{
		auto& windowSwapchain = window->GetSwapchain();

		if (m_renderTargets.contains(window))
		{
			RenderTarget& renderTarget = m_renderTargets.at(window);
			const bool requiresResize = windowSwapchain.GetWidth() != renderTarget.image->GetWidth() || windowSwapchain.GetHeight() != renderTarget.image->GetHeight();

			if (requiresResize)
			{
				renderTarget.image = CreateRenderTargetForWindow(window);
			}

			return renderTarget.image;
		}

		RenderTarget& renderTarget = m_renderTargets[window];
		renderTarget.image = CreateRenderTargetForWindow(window);

		return renderTarget.image;
	}

	void ImGuiRenderTargetManager::RemoveWindowRenderTarget(Window* window)
	{
		if (m_renderTargets.contains(window))
		{
			m_renderTargets.erase(window);
		}
	}

	RefPtr<RHI::Image> ImGuiRenderTargetManager::CreateRenderTargetForWindow(Window* window)
	{
		auto& windowSwapchain = window->GetSwapchain();

		RHI::ImageDesc desc{};
		desc.format = RHI::PixelFormat::R8G8B8A8_UNORM;
		desc.width = windowSwapchain.GetWidth();
		desc.height = windowSwapchain.GetHeight();
		desc.imageType = RHI::ResourceType::Image2D;
		desc.usage = RHI::ImageUsage::AttachmentStorage;
		desc.generateMips = false;
		desc.initializeImage = false;
		desc.debugName = "ImGui.RenderTarget";

		return RHI::Image::Create(desc);
	}
}
