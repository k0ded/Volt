#pragma once

#include <RHIModule/Images/Image.h>
#include <RHIModule/ResourceViewCache.h>

struct VkImage_T;

namespace Volt::RHI
{
	class Allocation;
	class GPUAllocator;

	class VulkanImage final : public Image
	{
	public:
		VulkanImage(const ImageDesc& specification, const void* data);
		VulkanImage(const SwapchainImageDesc& specification);
		~VulkanImage() override;

		/*
			Image Interface
		*/
		IntRef<ImageView> GetView(const ImageViewDesc& desc) override;
		VT_INLINE const uint32_t GetWidth() const override { return m_desc.width; }
		VT_INLINE const uint32_t GetHeight() const override { return m_desc.height; }
		VT_INLINE const uint32_t GetDepth() const override { return m_desc.depth; }
		VT_INLINE const bool IsSwapchainImage() const override { return m_isSwapchainImage; };
		VT_INLINE const ImageAspect GetImageAspect() const override { return m_imageAspect; }
		VT_INLINE const ImageDesc& GetDesc() const override { return m_desc; }

		/*
			RHIResource Interface
		*/
		VT_INLINE ResourceType GetType() const override { return m_desc.imageType; }
		void SetName(const String& name) override;
		StringView GetName() const override;
		uint64_t GetDeviceAddress() const override;
		const MemoryRequirement& GetMemoryRequirements() const override;
		uint64_t GetResourceByteSize() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		struct SwapchainImageData
		{
			VkImage_T* image = nullptr;
		};

		void Invalidate(const uint32_t width, const uint32_t height, const uint32_t depth, const void* data);
		void Release();

		void InvalidateSwapchainImage(const SwapchainImageDesc& specification);
		void TransitionToLayout(ImageLayout targetLayout);
		void InitializeWithData(const void* data);

		ImageDesc m_desc;
		SwapchainImageData m_swapchainImageData;
		ImageViewCache m_viewCache;

		Handle<Allocation> m_allocation;

		bool m_isSwapchainImage = false;

		ImageAspect m_imageAspect = ImageAspect::None;
	};
}
