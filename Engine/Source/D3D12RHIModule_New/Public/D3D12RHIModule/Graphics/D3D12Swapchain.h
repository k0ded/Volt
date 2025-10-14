#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Graphics/Swapchain.h>

struct IDXGISwapChain4;

namespace Volt::RHI
{
	class D3D12Swapchain final : public Swapchain
	{
	public:
		D3D12Swapchain(const SwapchainCreateInfo& createInfo);
		~D3D12Swapchain() override;

		void BeginFrame() override;
		void Present() override;
		void Resize(const uint32_t width, const uint32_t height, bool enableVSync) override;

		const uint32_t GetCurrentFrame() const override;
		const uint32_t GetWidth() const override;
		const uint32_t GetHeight() const override;
		const PixelFormat GetFormat() const override;
		RefPtr<Image> GetCurrentImage() const override;
		bool IsHDREnabled() const override;

		VT_NODISCARD ComPtr<ID3D12Resource> GetImageAtIndex(const uint32_t index) const { return m_perImageData.at(index).resource; }

	protected:
		void* GetHandleImpl() const override;

	private:
		struct PerImageData
		{
			ComPtr<ID3D12Resource> resource = nullptr;
			RefPtr<Image> imageReference;
		};

		void Invalidate(const uint32_t width, const uint32_t height);
		void Release();

		void CreateSwapchain(const uint32_t width, const uint32_t height);
		void GetNextFrameIndex();
		void GetSwapchainImages();

		uint32_t m_currentImageIndex = 0;
		uint32_t m_currentFrameIndex = 0;

		uint32_t m_width = 1280;
		uint32_t m_height = 720;

		bool m_VSyncEnabled = false;
		bool m_isHDREnabled = false;
		bool m_supportsTearing = false;

		SwapchainCreateInfo m_createInfo;

		Vector<RefPtr<CommandBuffer>> m_commandBuffers;
		Vector<PerImageData> m_perImageData;

		Vector<RefPtr<Fence>> m_renderFences;

		HWND m_windowHandle;
		ComPtr<IDXGISwapChain4> m_swapchain;
	};
}
