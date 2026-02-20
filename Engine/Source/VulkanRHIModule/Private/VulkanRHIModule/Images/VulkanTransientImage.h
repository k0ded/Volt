#pragma once

#include <RHIModule/Images/TransientImage.h>
#include <RHIModule/ResourceViewCache.h>

namespace Volt::RHI
{
	class VulkanTransientImage final : public TransientImage
	{
	public:
		VulkanTransientImage(const ImageDesc& desc);
		~VulkanTransientImage() override;

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

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateImage();

		ImageDesc m_desc;
		ImageViewCache m_viewCache;

		Handle<Allocation> m_allocation;
		ImageAspect m_imageAspect = ImageAspect::None;
	};
}
