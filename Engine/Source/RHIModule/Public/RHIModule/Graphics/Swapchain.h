#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include "RHIModule/Images/Image.h"
#include "RHIModule/Buffers/CommandBuffer.h"

struct GLFWwindow;

namespace Volt::RHI
{
	struct SwapchainCreateInfo
	{
		uint32_t width;
		uint32_t height;
		void* platformWindow;
		bool useHDRIfAvailable;
		bool enableVSync;
	};

	class VTRHI_API Swapchain : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(Swapchain);
		~Swapchain() override = default;

		virtual void BeginFrame() = 0;
		virtual void Present() = 0;
		virtual void Resize(const uint32_t width, const uint32_t height, bool enableVSync) = 0;

		VT_NODISCARD virtual const uint32_t GetCurrentFrame() const = 0;
		VT_NODISCARD virtual RefPtr<Image> GetCurrentImage() const = 0;
		VT_NODISCARD virtual const uint32_t GetWidth() const = 0;
		VT_NODISCARD virtual const uint32_t GetHeight() const = 0;
		VT_NODISCARD virtual const PixelFormat GetFormat() const = 0;
		VT_NODISCARD virtual bool IsHDREnabled() const = 0;

		static RefPtr<Swapchain> Create(const SwapchainCreateInfo& createInfo);

	protected:
		Swapchain() = default;
	};
}
