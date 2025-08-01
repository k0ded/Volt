#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Images/VulkanImage.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"

#include <RHIModule/Core/Profiling.h>
#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHIModule.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#ifdef VT_ENABLE_NV_AFTERMATH

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_Defines.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>

#include <RHIModule/Utility/NsightAftermathHelpers.h>

#endif

namespace Volt::RHI
{
	namespace Utility
	{
		inline static bool IsHDRColorSpace(ColorSpace colorSpace)
		{
			return colorSpace != ColorSpace::SRGB_NONLINEAR;
		}

		inline static uint32_t GetColorSpaceScore(ColorSpace colorSpace, bool useHDRIfAvailable)
		{
			uint32_t useHDR = uint32_t(useHDRIfAvailable);

			switch (colorSpace)
			{
				case ColorSpace::SRGB_NONLINEAR: return 1;
				case ColorSpace::DISPLAY_P3_NONLINEAR: return 2 * useHDR;
				case ColorSpace::EXTENDED_SRGB_LINEAR: return 1 * useHDR;
				case ColorSpace::DISPLAY_P3_LINEAR: return 2 * useHDR;
				case ColorSpace::DCI_P3_NONLINEAR: return 2 * useHDR;
				case ColorSpace::BT709_LINEAR: return 2 * useHDR;
				case ColorSpace::BT709_NONLINEAR: return 2 * useHDR;
				case ColorSpace::BT2020_LINEAR: return 2 * useHDR;
				case ColorSpace::HDR10_ST2084: return 2 * useHDR;
				case ColorSpace::DOLBYVISION: return 2 * useHDR;
				case ColorSpace::HDR10_HLG: return 2 * useHDR;
				case ColorSpace::ADOBERGB_LINEAR: return 2 * useHDR;
				case ColorSpace::ADOBERGB_NONLINEAR: return 2 * useHDR;
				case ColorSpace::PASS_THROUGH_EXT: return 2 * useHDR;
				case ColorSpace::EXTENDED_SRGB_NONLINEAR: return 2 * useHDR;
				case ColorSpace::DISPLAY_NATIVE_AMD: return 2 * useHDR;
			}

			return 0;
		}

		inline static uint32_t GetFormatScore(PixelFormat format, bool useHDRIfAvailable)
		{
			uint32_t useHDR = uint32_t(useHDRIfAvailable);

			switch (format)
			{
				case PixelFormat::B8G8R8A8_UNORM: return 2;
				case PixelFormat::B8G8R8A8_SRGB: return 1;
				case PixelFormat::R8G8B8A8_UNORM: return 3;
				case PixelFormat::R8G8B8A8_SRGB: return 2;
				case PixelFormat::R16G16B16A16_SFLOAT: return 10 * useHDR;
				case PixelFormat::A2B10G10R10_UNORM_PACK32: return 15 * useHDR;
			}

			return 0;
		}

		inline static VkSurfaceFormatKHR ChooseSwapchainFormat(const std::span<VulkanSwapchain::SurfaceFormat> swapchainFormats, bool useHDRIfAvailable)
		{
			VT_PROFILE_FUNCTION();

			VkSurfaceFormatKHR result{};

			uint32_t bestScore = 0;

			for (const auto& format : swapchainFormats)
			{
				uint32_t score = GetColorSpaceScore(format.colorSpace, useHDRIfAvailable) + GetFormatScore(format.format, useHDRIfAvailable);

				if (score >= bestScore)
				{
					result.format = Utility::VoltToVulkanFormat(format.format);
					result.colorSpace = Utility::VoltToVulkanColorSpace(format.colorSpace);

					bestScore = score;
				}
			}

			return result;
		}

		inline static VkPresentModeKHR ChooseSwapchainPresentMode(bool useVSync, const std::span<PresentMode> presentModes)
		{
			VT_PROFILE_FUNCTION();

			for (const auto& presentMode : presentModes)
			{
				if (useVSync && presentMode == PresentMode::FIFO)
				{
					return Utility::VoltToVulkanPresentMode(presentMode);
				}

				if (!useVSync && presentMode == PresentMode::Mailbox)
				{
					return Utility::VoltToVulkanPresentMode(presentMode);
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}
	}

	VulkanSwapchain::VulkanSwapchain(const SwapchainCreateInfo& createInfo)
		: m_createInfo(createInfo), m_VSyncEnabled(createInfo.enableVSync), m_width(createInfo.width), m_height(createInfo.height)
	{
		auto vulkanContext = GraphicsContext::Get().As<VulkanGraphicsContext>();
		auto& vulkanPhysicalDevice = GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>();

		VkInstance instance = vulkanContext->GetHandle<VkInstance>();
		VT_VK_CHECK(glfwCreateWindowSurface(instance, reinterpret_cast<GLFWwindow*>(createInfo.platformWindow), nullptr, &m_surface));

		const auto& queueFamilies = vulkanPhysicalDevice.GetQueueFamilies();

		VkBool32 supportsPresent = VK_FALSE;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vulkanPhysicalDevice.GetHandle<VkPhysicalDevice>(), queueFamilies.graphicsFamilyQueueIndex, m_surface, &supportsPresent));

		VT_ASSERT_MSG(supportsPresent, "Device does not have present support!");

		m_commandBuffers.resize(GetFramesInFlight());
		m_fences.resize(GetFramesInFlight());
		for (uint32_t i = 0; i < GetFramesInFlight(); i++)
		{
			m_commandBuffers[i] = CommandBuffer::Create();
			m_fences[i] = Fence::Create({ true });
		}
		 
		Invalidate(m_width, m_height, m_VSyncEnabled);
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		Release();
	}

	void VulkanSwapchain::BeginFrame()
	{
		VT_PROFILE_FUNCTION();

		if (m_swapchainNeedsRebuild)
		{
			QuerySwapchainCapabilities();
			Invalidate(m_width, m_height, m_VSyncEnabled);
			m_swapchainNeedsRebuild = false;
		}

		auto device = GraphicsContext::GetDevice();
		auto& frameData = m_perFrameInFlightData.at(m_currentFrame);

		m_fences.at(m_currentFrame)->WaitUntilSignaled();
		m_fences.at(m_currentFrame)->Reset();

		m_commandBuffers.at(m_currentFrame)->Begin();
		VkResult swapchainStatus = vkAcquireNextImageKHR(device->GetHandle<VkDevice>(), m_swapchain, 1000000000, frameData.presentSemaphore, nullptr, &m_currentImage);

		if (swapchainStatus == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_swapchainNeedsRebuild = true;
			return;
		}
		else if (swapchainStatus != VK_SUCCESS && swapchainStatus != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("Failed to acquire swapchain image!");
		}
	}

	void VulkanSwapchain::Present()
	{
		VT_PROFILE_FUNCTION();

		{
			ResourceBarrierInfo barrier{};
			barrier.type = BarrierType::Image;
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), m_perImageData.at(m_currentImage).imageReference);
			barrier.imageBarrier().dstAccess = BarrierAccess::None;
			barrier.imageBarrier().dstStage = BarrierStage::AllGraphics;
			barrier.imageBarrier().dstLayout = ImageLayout::Present;
			barrier.imageBarrier().resource = m_perImageData.at(m_currentImage).imageReference;

			m_commandBuffers.at(m_currentFrame)->ResourceBarrier({ barrier });
		}

		m_commandBuffers.at(m_currentFrame)->End();

		if (m_swapchainNeedsRebuild)
		{
			return;
		}

		auto& frameData = m_perFrameInFlightData.at(m_currentFrame);
		const auto deviceQueue = GraphicsContext::GetDevice()->GetDeviceQueue(QueueType::Graphics);

		VulkanDeviceQueue& vkQueue = deviceQueue->AsRef<VulkanDeviceQueue>();

		// Queue Submit
		{
			VkCommandBuffer cmdBuffer = m_commandBuffers.at(m_currentFrame)->GetHandle<VkCommandBuffer>();
			VkFence fence = m_fences.at(m_currentFrame)->GetHandle<VkFence>();

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;

			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &m_perFrameInFlightData.at(m_currentImage).renderSemaphore;

			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &frameData.presentSemaphore;

			const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			submitInfo.pWaitDstStageMask = &waitStage;

			vkQueue.AquireLock();
			VT_VK_CHECK(vkQueueSubmit(deviceQueue->GetHandle<VkQueue>(), 1, &submitInfo, fence));
			vkQueue.ReleaseLock();
		
			m_fences.at(m_currentFrame)->As<VulkanFence>()->MarkAsExecuted();
		}

		// Present to screen
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_swapchain;

			presentInfo.pWaitSemaphores = &m_perFrameInFlightData.at(m_currentImage).renderSemaphore;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pImageIndices = &m_currentImage;

			vkQueue.AquireLock();
			VkResult presentResult = vkQueuePresentKHR(deviceQueue->GetHandle<VkQueue>(), &presentInfo);
			vkQueue.ReleaseLock();

			GetNextFrameIndex();

			if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
			{
				m_swapchainNeedsRebuild = true;
				return;
			}
			else if (presentResult != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to present swapchain image!");
			}
		}

	}

	void VulkanSwapchain::Resize(const uint32_t width, const uint32_t height, bool enableVSync)
	{
		if (!m_swapchain)
		{
			return;
		}

		if (m_width == width && m_height == height && m_VSyncEnabled == enableVSync)
		{
			return;
		}

		m_width = width;
		m_height = height;
		m_VSyncEnabled = enableVSync;

		QuerySwapchainCapabilities();
		CreateSwapchain(width, height, enableVSync);
	}

	const uint32_t VulkanSwapchain::GetCurrentFrame() const
	{
		return m_currentFrame;
	}

	const uint32_t VulkanSwapchain::GetWidth() const
	{
		return m_width;
	}

	const uint32_t VulkanSwapchain::GetHeight() const
	{
		return m_height;
	}

	const uint32_t VulkanSwapchain::GetFramesInFlight() const
	{
		return VulkanSwapchain::MAX_FRAMES_IN_FLIGHT;
	}

	const PixelFormat VulkanSwapchain::GetFormat() const
	{
		return m_swapchainFormat;
	}

	RefPtr<Image> VulkanSwapchain::GetCurrentImage() const
	{
		const auto& data = m_perImageData.at(m_currentImage);
		return data.imageReference;
	}

	bool VulkanSwapchain::IsHDREnabled() const
	{
		return m_isHDREnabled;
	}

	void* VulkanSwapchain::GetHandleImpl() const
	{
		return m_swapchain;
	}

	void VulkanSwapchain::Invalidate(const uint32_t width, const uint32_t height, bool enableVSync)
	{
		VT_PROFILE_FUNCTION();

		m_width = width;
		m_height = height;
		m_VSyncEnabled = enableVSync;

		QuerySwapchainCapabilities();

		CreateSwapchain(width, height, enableVSync);
		CreateSyncObjects();
	}

	void VulkanSwapchain::Release()
	{
		if (!m_swapchain)
		{
			return;
		}

		
		auto vulkanFence = m_fences.at(m_currentFrame).As<VulkanFence>();

		if (vulkanFence->HasBeenExecuted())
		{
			m_fences.at(m_currentFrame)->WaitUntilSignaled();
		}

		RHIModule::GetInstance().DestroyResource([perFrameInFlightData = m_perFrameInFlightData, swapchain = m_swapchain, surface = m_surface]()
		{
			auto device = GraphicsContext::GetDevice();

			for (auto& perFrameData : perFrameInFlightData)
			{
				vkDestroySemaphore(device->GetHandle<VkDevice>(), perFrameData.presentSemaphore, nullptr);
				vkDestroySemaphore(device->GetHandle<VkDevice>(), perFrameData.renderSemaphore, nullptr);
			}

			vkDestroySwapchainKHR(device->GetHandle<VkDevice>(), swapchain, nullptr);
			vkDestroySurfaceKHR(GraphicsContext::Get().GetHandle<VkInstance>(), surface, nullptr);
		});

		m_perFrameInFlightData.clear();
		m_perImageData.clear();
	}

	void VulkanSwapchain::QuerySwapchainCapabilities()
	{
		VT_PROFILE_FUNCTION();

		auto physicalDevice = GraphicsContext::GetPhysicalDevice();

		VkSurfaceCapabilitiesKHR capabilities{};
		VT_VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &capabilities));

		m_capabilities.minImageCount = capabilities.minImageCount;
		m_capabilities.maxImageCount = capabilities.maxImageCount;

		m_capabilities.minImageExtent.width = capabilities.minImageExtent.width;
		m_capabilities.minImageExtent.height = capabilities.minImageExtent.height;
		m_capabilities.maxImageExtent.width = capabilities.maxImageExtent.width;
		m_capabilities.maxImageExtent.height = capabilities.maxImageExtent.height;

		uint32_t formatCount = 0;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &formatCount, nullptr));

		if (formatCount > 0)
		{
			m_capabilities.surfaceFormats.resize(formatCount);
			VT_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &formatCount, (VkSurfaceFormatKHR*)m_capabilities.surfaceFormats.data()));
		}

		uint32_t presentModeCount = 0;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &presentModeCount, nullptr));

		if (presentModeCount > 0)
		{
			m_capabilities.presentModes.resize(presentModeCount);
			VT_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &presentModeCount, (VkPresentModeKHR*)m_capabilities.presentModes.data()));
		}
	}

	void VulkanSwapchain::CreateSwapchain(const uint32_t width, const uint32_t height, bool enableVSync)
	{
		VT_PROFILE_FUNCTION();

		const VkSurfaceFormatKHR surfaceFormat = Utility::ChooseSwapchainFormat(m_capabilities.surfaceFormats, m_createInfo.useHDRIfAvailable);
		if (surfaceFormat.colorSpace )
		{
		}

		const VkPresentModeKHR presentMode = Utility::ChooseSwapchainPresentMode(enableVSync, m_capabilities.presentModes);

		m_totalImageCount = m_capabilities.minImageCount + 1;
		if (m_capabilities.maxImageCount > 0 && m_totalImageCount > m_capabilities.maxImageCount)
		{
			m_totalImageCount = m_capabilities.maxImageCount;
		}

		// Make sure the requested size is within the capabilities of the swapchain
		m_width = std::clamp(width, m_capabilities.minImageExtent.width, m_capabilities.maxImageExtent.width);
		m_height = std::clamp(height, m_capabilities.minImageExtent.height, m_capabilities.maxImageExtent.height);

		VkSwapchainKHR oldSwapchain = m_swapchain;

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_surface;
		swapchainCreateInfo.minImageCount = m_totalImageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent.width = m_width;
		swapchainCreateInfo.imageExtent.height = m_height;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		auto device = GraphicsContext::GetDevice();

		{
			VT_PROFILE_SCOPE("vkCreateSwapchainKHR");
			VT_VK_CHECK(vkCreateSwapchainKHR(device->GetHandle<VkDevice>(), &swapchainCreateInfo, nullptr, &m_swapchain));
		}

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (auto& imageData : m_perImageData)
			{
				imageData.imageReference = nullptr;
			}

			RHIModule::GetInstance().DestroyResource([oldSwapchain]() 
			{
				auto device = GraphicsContext::GetDevice();
				vkDestroySwapchainKHR(device->GetHandle<VkDevice>(), oldSwapchain, nullptr);
			});

		}

		Vector<VkImage> images{};
		{
			VT_PROFILE_SCOPE("Get Images");
			VT_VK_CHECK(vkGetSwapchainImagesKHR(device->GetHandle<VkDevice>(), m_swapchain, &m_totalImageCount, nullptr));

			m_perImageData.clear();
			m_perImageData.resize(m_totalImageCount);
			images.resize(m_totalImageCount);

			VT_VK_CHECK(vkGetSwapchainImagesKHR(device->GetHandle<VkDevice>(), m_swapchain, &m_totalImageCount, images.data()));
		}

		m_swapchainFormat = Utility::VulkanToVoltFormat(surfaceFormat.format);

		{
			VT_PROFILE_SCOPE("Create Images");

			for (size_t i = 0; i < m_perImageData.size(); i++)
			{
				m_perImageData[i].image = images.at(i);

				SwapchainImageDesc spec{};
				spec.swapchain = this;
				spec.imageIndex = static_cast<uint32_t>(i);

				m_perImageData[i].imageReference = Image::Create(spec);
			}
		}
	}

	void VulkanSwapchain::CreateSyncObjects()
	{
		VT_PROFILE_FUNCTION();

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		auto device = GraphicsContext::GetDevice();

		m_perFrameInFlightData.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (auto& frameData : m_perFrameInFlightData)
		{
			VT_VK_CHECK(vkCreateSemaphore(device->GetHandle<VkDevice>(), &semaphoreInfo, nullptr, &frameData.presentSemaphore));
			VT_VK_CHECK(vkCreateSemaphore(device->GetHandle<VkDevice>(), &semaphoreInfo, nullptr, &frameData.renderSemaphore));
		}
	}

	void VulkanSwapchain::GetNextFrameIndex()
	{
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
}
