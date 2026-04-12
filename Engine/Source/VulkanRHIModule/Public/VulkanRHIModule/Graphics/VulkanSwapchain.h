#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Synchronization/Semaphore.h>

#include <CoreUtilities/Profiling/Profiling.h>

struct VkSwapchainKHR_T;
struct VkRenderPass_T;
struct VkSurfaceKHR_T;

struct VkFence_T;
struct VkSemaphore_T;
struct VkCommandPool_T;
struct VkCommandBuffer_T;

struct VkImage_T;
struct VkImageView_T;
struct VkFramebuffer_T;

struct GLFWwindow;

namespace Volt::RHI
{
	class VulkanSwapchain final : public Swapchain
	{
	public:
		struct SurfaceFormat
		{
			PixelFormat format;
			ColorSpace colorSpace;
		};
		
		VulkanSwapchain(const SwapchainCreateInfo& createInfo);
		~VulkanSwapchain() override;

		void BeginFrame() override;
		void Present() override;
		void Resize(const uint32_t width, const uint32_t height, bool enableVSync) override;

		const uint32_t GetCurrentFrame() const override;
		const uint32_t GetWidth() const override;
		const uint32_t GetHeight() const override;
		const PixelFormat GetFormat() const override;
		IntRef<Image> GetCurrentImage() const override;
		bool IsHDREnabled() const override;

		inline VkImage_T* GetImageAtIndex(const uint32_t index) const { return m_perImageData.at(index).image; }
		inline IntRef<Semaphore> GetAcquireSemaphore() const { return m_perFrameInFlightData[m_currentFrameIndex].acquireSemaphore; }

	protected:
		void* GetHandleImpl() const override;
	
	private:
		void Invalidate(const uint32_t width, const uint32_t height, bool enableVSync);
		void Release();

		void QuerySwapchainCapabilities();

		VkSwapchainKHR_T* CreateSwapchain(const uint32_t width, const uint32_t height, bool enableVSync);
		void CreateSyncObjects();
		void CreateRenderSemaphores();
		void CreateWindowSurface(void* platformWindow, void* platformInstance);
		void CreateSwapchainImage(uint32_t imageIndex);

		void ReleasePreviousSwapchain(VkSwapchainKHR_T* swapchain);

		void GetNextFrameIndex();
		VkFence_T* GetLastSubmittedPresentFence();

		uint32_t m_currentImageIndex = 0;
		uint32_t m_currentFrameIndex = 0;
		uint32_t m_lastSubmittedFence;

		uint32_t m_width = 1280;
		uint32_t m_height = 720;

		bool m_VSyncEnabled = false;
		bool m_isHDREnabled = false;
		bool m_swapchainNeedsRebuild = false;

		uint32_t m_totalImageCount = 0;

		struct PerFrameInFlightData
		{
			IntRef<Semaphore> acquireSemaphore;
			VkFence_T* renderFence = nullptr;
			VkFence_T* presentFence = nullptr;
		};

		struct PerImageData
		{
			VkImage_T* image = nullptr;
			VkSemaphore_T* renderSemaphore = nullptr;
			IntRef<Image> imageReference;
		};

		struct SwapchainCapabilities
		{
			uint32_t minImageCount = 0;
			uint32_t maxImageCount = 0;

			Extent2D minImageExtent{};
			Extent2D maxImageExtent{};
			Extent2D currentExtent{};

			uint32_t compositeAlphaFlags = 0;

			Vector<PresentMode> presentModes{};
			Vector<SurfaceFormat> surfaceFormats{};
		};

		SwapchainCapabilities m_capabilities{};
		SwapchainCreateInfo m_createInfo{};

		Vector<IntRef<CommandBuffer>> m_commandBuffers;
		Vector<PerFrameInFlightData> m_perFrameInFlightData{};
		Vector<PerImageData> m_perImageData{};

		VkSwapchainKHR_T* m_swapchain = nullptr;
		VkSurfaceKHR_T* m_surface = nullptr;

		std::mutex m_swapchainMutex;

		PixelFormat m_swapchainFormat = PixelFormat::UNDEFINED;
	};
}
