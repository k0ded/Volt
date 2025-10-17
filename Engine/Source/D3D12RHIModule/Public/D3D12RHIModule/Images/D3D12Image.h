#pragma once

#include <RHIModule/Images/Image.h>

struct ID3D12Resource;

namespace Volt::RHI
{
	class D3D12Image final : public Image
	{
	public:
		D3D12Image(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator);
		D3D12Image(const SwapchainImageDesc& specification);
		~D3D12Image() override;

		void Invalidate(const uint32_t width, const uint32_t height, const uint32_t depth, const void* data) override;
		void Release() override;
		void GenerateMips() override;

		RefPtr<ImageView> GetView(const ImageViewDesc& desc) override;

		VT_INLINE const uint32_t GetWidth() const override { return m_desc.width; }
		VT_INLINE const uint32_t GetHeight() const override { return m_desc.height; }
		VT_INLINE const uint32_t GetDepth() const override { return m_desc.depth; }
		VT_INLINE const uint32_t GetMipCount() const override { return m_desc.mips; }
		VT_INLINE const uint32_t GetLayerCount() const override { return m_desc.layers; }
		VT_INLINE const PixelFormat GetFormat() const override { return m_desc.format; }
		VT_INLINE const ImageUsage GetUsage() const override { return m_desc.usage; }
		const uint32_t CalculateMipCount() const override;
		VT_INLINE const bool IsSwapchainImage() const override { return m_isSwapchainImage; };
		VT_INLINE const ImageAspect GetImageAspect() const override { return m_imageAspect; }
		VT_INLINE const ImageDesc& GetDesc() const override { return m_desc; }

		VT_INLINE ResourceType GetType() const override { return m_desc.imageType; }
		void SetName(const std::string& name) override;
		std::string_view GetName() const override;
		const uint64_t GetDeviceAddress() const override;
		const uint64_t GetByteSize() const override;

	protected:
		void* GetHandleImpl() const override;
		Buffer ReadPixelInternal(const uint32_t x, const uint32_t y, const uint32_t z, const size_t stride) override;

	private:
		struct SwapchainImageData
		{
			ID3D12Resource* image = nullptr;
		};

		void InvalidateSwapchainImage(const SwapchainImageDesc& specification);
		void TransitionToLayout(ImageLayout targetLayout);
		void InitializeWithData(const void* data);

		ImageDesc m_desc;
		SwapchainImageData m_swapchainImageData;

		Handle<Allocation> m_allocation;
		RawPtr<GPUAllocator> m_allocator;

		bool m_isSwapchainImage = false;

		ImageAspect m_imageAspect = ImageAspect::None;
	};
}
