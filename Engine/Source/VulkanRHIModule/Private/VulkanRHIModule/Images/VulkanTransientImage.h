#pragma once

#include <RHIModule/Images/TransientImage.h>
#include <RHIModule/ResourceViewCache.h>

struct VkImage_T;

namespace Volt::RHI
{
	class VulkanTransientImage final : public TransientImage
	{
	public:
		VulkanTransientImage(const ImageDesc& desc);
		~VulkanTransientImage() override;

		/*
		* TransientImage Interface
		*/
		void BindMemory(RefPtr<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset) override;

		/*
		* Image Interface
		*/
		RefPtr<ImageView> GetView(const ImageViewDesc& desc) override;
		VT_INLINE const uint32_t GetWidth() const override { return m_desc.width; }
		VT_INLINE const uint32_t GetHeight() const override { return m_desc.height; }
		VT_INLINE const uint32_t GetDepth() const override { return m_desc.depth; }
		VT_INLINE const bool IsSwapchainImage() const override { return false; };
		VT_INLINE const ImageAspect GetImageAspect() const override { return m_imageAspect; }
		VT_INLINE const ImageDesc& GetDesc() const override { return m_desc; }

		/*
		* RHIResource Interface
		*/
		VT_INLINE ResourceType GetType() const override { return m_desc.imageType; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		uint64_t GetDeviceAddress() const override;
		const MemoryRequirement& GetMemoryRequirements() const override;
		uint64_t GetResourceByteSize() const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateImage();

		ImageDesc m_desc;
		ImageViewCache m_viewCache;

		ImageAspect m_imageAspect = ImageAspect::None;

		VkImage_T* m_imageHandle;
		uint64_t m_deviceAddress = 0;
		MemoryRequirement m_memoryRequirements;
	};
}
