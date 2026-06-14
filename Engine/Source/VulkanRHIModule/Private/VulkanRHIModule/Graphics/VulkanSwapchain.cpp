#include "vkpch.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Images/VulkanImage.h"

#include "VulkanRHIModule/VulkanRHISubmissionThread.h"
#include "VulkanRHIModule/VulkanResourceCast.h"

#include <RHIModule/Core/Profiling.h>
#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/RHICapabilities.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

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
		: m_width(createInfo.width), 
		m_height(createInfo.height),
		m_VSyncEnabled(createInfo.enableVSync),
		m_createInfo(createInfo)
	{
		auto& vulkanPhysicalDevice = GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>();

		CreateWindowSurface(createInfo.platformWindow, createInfo.platformHandle);

		const auto& queueFamilies = vulkanPhysicalDevice.GetQueueFamilies();

		VkBool32 supportsPresent = VK_FALSE;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(vulkanPhysicalDevice.GetHandle<VkPhysicalDevice>(), queueFamilies.graphicsFamilyQueueIndex, m_surface, &supportsPresent));

		VT_ASSERT_MSG(supportsPresent, "Device does not have present support!");

		m_commandBuffers.resize(RHI::RHICapabilities::NumFramesInFlight);
		for (uint32_t i = 0; i < RHI::RHICapabilities::NumFramesInFlight; i++)
		{
			m_commandBuffers[i] = CommandBuffer::Create();
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
		auto& frameData = m_perFrameInFlightData.at(m_currentFrameIndex);

		vkWaitForFences(device->GetHandle<VkDevice>(), 1, &frameData.renderFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device->GetHandle<VkDevice>(), 1, &frameData.renderFence);

		VkResult swapchainStatus;
		{
			VT_PROFILE_SCOPE("AcquireNextImage");

			// Reset the 'wait' of the semaphore
			VulkanSemaphore* vkSemaphore = ResourceCast(frameData.acquireSemaphore.GetRaw());
			vkSemaphore->ResetWait();

			m_swapchainMutex.lock();
			swapchainStatus = vkAcquireNextImageKHR(device->GetHandle<VkDevice>(), m_swapchain, 1000000000, frameData.acquireSemaphore->GetHandle<VkSemaphore>(), nullptr, &m_currentImageIndex);
			m_swapchainMutex.unlock();
		}

		if (swapchainStatus == VK_SUCCESS || swapchainStatus == VK_SUBOPTIMAL_KHR)
		{
			if (m_perImageData[m_currentImageIndex].imageReference == nullptr)
			{
				CreateSwapchainImage(m_currentImageIndex);
			}
		}

		if (swapchainStatus == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_swapchainNeedsRebuild = true;
			return;
		}
		else if (swapchainStatus != VK_SUCCESS && swapchainStatus != VK_SUBOPTIMAL_KHR)
		{
			VT_ENSURE_NO_ENTRY();
		}
	}

	void VulkanSwapchain::Present()
	{
		VT_PROFILE_FUNCTION();

		// #Note: This can happen because windows may be created after begin frame,
		//		  but before present, meaning that the swapchain images has not beem created yet.
		if (m_perImageData[m_currentImageIndex].imageReference == nullptr)
		{
			return;
		}

		m_commandBuffers.at(m_currentFrameIndex)->Begin();

		{
			ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsImageBarrier();
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), m_perImageData.at(m_currentImageIndex).imageReference);
			barrier.imageBarrier().dstAccess = BarrierAccess::None;
			barrier.imageBarrier().dstStage = BarrierStage::AllGraphics;
			barrier.imageBarrier().dstLayout = ImageLayout::Present;
			barrier.imageBarrier().resource = m_perImageData.at(m_currentImageIndex).imageReference;

			m_commandBuffers.at(m_currentFrameIndex)->ResourceBarrier({ barrier });
		}

		m_commandBuffers.at(m_currentFrameIndex)->End();

		if (m_swapchainNeedsRebuild)
		{
			return;
		}

		PerFrameInFlightData& frameData = m_perFrameInFlightData.at(m_currentFrameIndex);
		PerImageData& imageData = m_perImageData.at(m_currentImageIndex);

		VulkanRHISubmissionThread* submissionThread = ResourceCast(&RHIModule::GetInstance().GetSubmissionThread());
		{
			VulkanSemaphore* vkSemaphore = ResourceCast(frameData.acquireSemaphore.GetRaw());
			VkSemaphore semaphore = nullptr;

			if (vkSemaphore->TryGetWait())
			{
				semaphore = vkSemaphore->GetHandle<VkSemaphore>();
			}

			submissionThread->QueueSwapchainSubmit(
				semaphore,
				imageData.renderSemaphore,
				frameData.renderFence,
				m_commandBuffers.at(m_currentFrameIndex)->GetHandle<VkCommandBuffer>()
			);

			m_lastSubmittedFence = m_currentFrameIndex;
		}

		{
			auto device = GraphicsContext::GetDevice();

			// Reset the present fence
			vkWaitForFences(device->GetHandle<VkDevice>(), 1, &frameData.presentFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device->GetHandle<VkDevice>(), 1, &frameData.presentFence);

			submissionThread->QueueSwapchainPresent(
				m_swapchain,
				imageData.renderSemaphore,
				frameData.presentFence,
				m_currentImageIndex,
				&m_swapchainMutex
			);

			GetNextFrameIndex();
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
		
		VkSwapchainKHR oldSwapchain = CreateSwapchain(width, height, enableVSync);
		
		if (oldSwapchain)
		{
			ReleasePreviousSwapchain(oldSwapchain);
		}
		
		CreateRenderSemaphores();
	}

	const uint32_t VulkanSwapchain::GetCurrentFrame() const
	{
		return m_currentFrameIndex;
	}

	const uint32_t VulkanSwapchain::GetWidth() const
	{
		return m_width;
	}

	const uint32_t VulkanSwapchain::GetHeight() const
	{
		return m_height;
	}

	const PixelFormat VulkanSwapchain::GetFormat() const
	{
		return m_swapchainFormat;
	}

	IntRef<Image> VulkanSwapchain::GetCurrentImage() const
	{
		const auto& data = m_perImageData.at(m_currentImageIndex);
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

		m_lastSubmittedFence = RHI::RHICapabilities::NumFramesInFlight;

		QuerySwapchainCapabilities();

		CreateSwapchain(width, height, enableVSync);
		CreateSyncObjects();
		CreateRenderSemaphores();
	}

	void VulkanSwapchain::Release()
	{
		if (!m_swapchain)
		{
			return;
		}

		struct TempData
		{
			VkSemaphore presentSemaphore;
		};

		Vector<TempData> tempData;
		tempData.resize(m_perImageData.size());

		for (size_t i = 0; i < m_perImageData.size(); ++i)
		{
			tempData[i].presentSemaphore = m_perImageData[i].renderSemaphore;
		}

		for (auto& perFrameInFlightData : m_perFrameInFlightData)
		{
			perFrameInFlightData.acquireSemaphore.Reset();
		}

		RHIModule::GetInstance().DestroyResource([perFrameInFlightData = m_perFrameInFlightData, tempData, swapchain = m_swapchain, surface = m_surface]() mutable
		{
			auto device = GraphicsContext::GetDevice();
			VkDevice vkDevice = device->GetHandle<VkDevice>();

			for (auto& perFrameData : perFrameInFlightData)
			{
				vkWaitForFences(vkDevice, 1, &perFrameData.presentFence, VK_TRUE, UINT64_MAX);
			}

			for (const TempData& data : tempData)
			{
				vkDestroySemaphore(vkDevice, data.presentSemaphore, VT_VULKAN_ALLOCATOR);
			}

			for (auto& perFrameData : perFrameInFlightData)
			{
				vkDestroyFence(vkDevice, perFrameData.renderFence, VT_VULKAN_ALLOCATOR);
				vkDestroyFence(vkDevice, perFrameData.presentFence, VT_VULKAN_ALLOCATOR);
			}

			vkDestroySwapchainKHR(vkDevice, swapchain, VT_VULKAN_ALLOCATOR);
			vkDestroySurfaceKHR(GraphicsContext::Get().GetHandle<VkInstance>(), surface, nullptr); 
		}, nullptr);

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

		m_capabilities.currentExtent.width = capabilities.currentExtent.width;
		m_capabilities.currentExtent.height = capabilities.currentExtent.height;

		if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		{
			m_capabilities.compositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		}
		else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
		{
			m_capabilities.compositeAlphaFlags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		}
		else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
		{
			m_capabilities.compositeAlphaFlags = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
		}
		else if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
		{
			m_capabilities.compositeAlphaFlags = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
		}

		uint32_t formatCount = 0;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &formatCount, nullptr));

		if (formatCount > 0)
		{
			m_capabilities.surfaceFormats.resize(formatCount);
			static_assert(sizeof(VkSurfaceFormatKHR) == sizeof(SurfaceFormat));

			VT_VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &formatCount, (VkSurfaceFormatKHR*)m_capabilities.surfaceFormats.data()));
		}

		uint32_t presentModeCount = 0;
		VT_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &presentModeCount, nullptr));

		if (presentModeCount > 0)
		{
			Vector<VkPresentModeKHR> vkPresentModes;
			vkPresentModes.resize(presentModeCount);

			VT_VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice->GetHandle<VkPhysicalDevice>(), m_surface, &presentModeCount, vkPresentModes.data()));

			m_capabilities.presentModes.resize(presentModeCount);

			for (VkPresentModeKHR presentMode : vkPresentModes)
			{
				switch (presentMode)
				{
					case VK_PRESENT_MODE_IMMEDIATE_KHR: m_capabilities.presentModes.emplace_back(PresentMode::Immediate); break;
					case VK_PRESENT_MODE_MAILBOX_KHR: m_capabilities.presentModes.emplace_back(PresentMode::Mailbox); break;
					case VK_PRESENT_MODE_FIFO_KHR: m_capabilities.presentModes.emplace_back(PresentMode::FIFO); break;
					case VK_PRESENT_MODE_FIFO_RELAXED_KHR: m_capabilities.presentModes.emplace_back(PresentMode::FIFORelaxed); break;
				}
			}
		}
	}

	VkSwapchainKHR_T* VulkanSwapchain::CreateSwapchain(const uint32_t width, const uint32_t height, bool enableVSync)
	{
		VT_PROFILE_FUNCTION();

		const VkSurfaceFormatKHR surfaceFormat = Utility::ChooseSwapchainFormat(m_capabilities.surfaceFormats, m_createInfo.useHDRIfAvailable);

		if (surfaceFormat.format == VK_FORMAT_R16G16B16A16_SFLOAT ||
			surfaceFormat.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ||
			surfaceFormat.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		{
			m_isHDREnabled = true;
		}
		else
		{
			m_isHDREnabled = false;
		}

		const VkPresentModeKHR presentMode = Utility::ChooseSwapchainPresentMode(enableVSync, m_capabilities.presentModes);

		m_totalImageCount = m_capabilities.minImageCount + 1;
		if (m_capabilities.maxImageCount > 0 && m_totalImageCount > m_capabilities.maxImageCount)
		{
			m_totalImageCount = m_capabilities.maxImageCount;
		}

		// Make sure the requested size is within the capabilities of the swapchain
		m_width = m_capabilities.currentExtent.width;
		m_height = m_capabilities.currentExtent.height;

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
		swapchainCreateInfo.compositeAlpha = static_cast<VkCompositeAlphaFlagBitsKHR>(m_capabilities.compositeAlphaFlags);
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.oldSwapchain = oldSwapchain;

		swapchainCreateInfo.flags = VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT;

		auto device = GraphicsContext::GetDevice();

		{
			VT_PROFILE_SCOPE("vkCreateSwapchainKHR");
			VT_VK_CHECK(vkCreateSwapchainKHR(device->GetHandle<VkDevice>(), &swapchainCreateInfo, VT_VULKAN_ALLOCATOR, &m_swapchain));
		}

		if (oldSwapchain != VK_NULL_HANDLE)
		{
			for (auto& imageData : m_perImageData)
			{
				imageData.imageReference = nullptr;
			}
		}

		Vector<VkImage> images{};
		{
			VT_PROFILE_SCOPE("Get Images");
			VT_VK_CHECK(vkGetSwapchainImagesKHR(device->GetHandle<VkDevice>(), m_swapchain, &m_totalImageCount, nullptr));

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
			}
		}

		return oldSwapchain;
	}

	void VulkanSwapchain::CreateSyncObjects()
	{
		VT_PROFILE_FUNCTION();

		auto device = GraphicsContext::GetDevice();

		m_perFrameInFlightData.resize(RHI::RHICapabilities::NumFramesInFlight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkDevice vkDevice = device->GetHandle<VkDevice>();

		for (auto& frameData : m_perFrameInFlightData)
		{
			frameData.acquireSemaphore = Semaphore::Create();
			VT_VK_CHECK(vkCreateFence(vkDevice, &fenceCreateInfo, VT_VULKAN_ALLOCATOR, &frameData.renderFence));
			VT_VK_CHECK(vkCreateFence(vkDevice, &fenceCreateInfo, VT_VULKAN_ALLOCATOR, &frameData.presentFence));
		}
	}

	void VulkanSwapchain::GetNextFrameIndex()
	{
		m_currentFrameIndex = (m_currentFrameIndex + 1) % RHI::RHICapabilities::NumFramesInFlight;
	}

	void VulkanSwapchain::CreateWindowSurface(void* platformWindow, void* platformInstance)
	{
#ifdef VT_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.pNext = nullptr;
		createInfo.hinstance = static_cast<HINSTANCE>(platformInstance);
		createInfo.hwnd = static_cast<HWND>(platformWindow);
		createInfo.flags = 0;

		auto vulkanContext = GraphicsContext::Get().As<VulkanGraphicsContext>();
		VkInstance instance = vulkanContext->GetHandle<VkInstance>();

		vkCreateWin32SurfaceKHR(instance, &createInfo, VT_VULKAN_ALLOCATOR, &m_surface);
#endif
	}

	void VulkanSwapchain::CreateRenderSemaphores()
	{
		auto device = GraphicsContext::GetDevice();
		VkDevice vkDevice = device->GetHandle<VkDevice>();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (auto& imageData : m_perImageData)
		{
			VT_VK_CHECK(vkCreateSemaphore(vkDevice, &semaphoreInfo, VT_VULKAN_ALLOCATOR, &imageData.renderSemaphore));
		}
	}

	void VulkanSwapchain::CreateSwapchainImage(uint32_t imageIndex)
	{
		SwapchainImageDesc spec{};
		spec.swapchain = this;
		spec.imageIndex = imageIndex;

		m_perImageData[imageIndex].imageReference = Image::Create(spec);
	}

	VkFence_T* VulkanSwapchain::GetLastSubmittedPresentFence()
	{
		if (m_lastSubmittedFence < RHI::RHICapabilities::NumFramesInFlight)
		{
			return m_perFrameInFlightData[m_lastSubmittedFence].presentFence;
		}

		return nullptr;
	}

	void VulkanSwapchain::ReleasePreviousSwapchain(VkSwapchainKHR swapchain)
	{
		Vector<VkSemaphore> tempData;
		tempData.resize(m_perImageData.size());

		for (size_t i = 0; i < m_perImageData.size(); ++i)
		{
			tempData[i] = m_perImageData[i].renderSemaphore;
		}

		RHIModule::GetInstance().DestroyResource([tempData, presentFence = GetLastSubmittedPresentFence(), swapchain]()
		{
			auto device = GraphicsContext::GetDevice();
			VkDevice vkDevice = device->GetHandle<VkDevice>();

			if (presentFence)
			{
				vkWaitForFences(vkDevice, 1, &presentFence, VK_TRUE, UINT64_MAX);
			}

			for (VkSemaphore presentSemaphore : tempData)
			{
				vkDestroySemaphore(vkDevice, presentSemaphore, VT_VULKAN_ALLOCATOR);
			}

			vkDestroySwapchainKHR(vkDevice, swapchain, VT_VULKAN_ALLOCATOR);

		}, nullptr);
	}
}
