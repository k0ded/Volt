#pragma once

#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"

#include <RHIModule/Images/ImageView.h>

namespace Volt::RHI
{
	class D3D12ImageView final : public ImageView
	{
	public:
		D3D12ImageView(const ImageViewDesc& specification, RawPtr<Image> image);
		~D3D12ImageView() override;

		const PixelFormat GetFormat() const;
		const ImageAspect GetImageAspect() const override;
		const uint64_t GetDeviceAddress() const override;
		const ImageUsage GetImageUsage() const override;
		const ImageViewType GetViewType() const override;
		const ImageViewDesc& GetDesc() const override;
		RawPtr<Image> GetImage() const override;
		const bool IsSwapchainView() const override;

		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetRTVDSVDescriptor() const { return m_rtvDsvDescriptor; }
		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetSRVDescriptor() const { return m_srvDescriptor; }
		VT_NODISCARD VT_INLINE const D3D12DescriptorPointer& GetUAVDescriptor() const { return m_uavDescriptor; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void CreateRTVDSV();
		void CreateSRV();
		void CreateUAV();

		ImageViewDesc m_desc{};
		RawPtr<Image> m_image;

		PixelFormat m_format;
		ImageAspect m_imageAspect;
		ImageUsage m_imageUsage;
		bool m_isSwapchainImage;

		D3D12ViewType m_viewUsage;
		D3D12DescriptorPointer m_rtvDsvDescriptor;
		D3D12DescriptorPointer m_srvDescriptor;
		D3D12DescriptorPointer m_uavDescriptor;
	};
}
