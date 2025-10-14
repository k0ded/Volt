#include "dxpch.h"

#include "D3D12RHIModule/Images/D3D12ImageView.h"
#include "D3D12RHIModule/Common/D3D12Helpers.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/RHIModule.h>
#include <CoreUtilities/EnumUtils.h>

namespace Volt::RHI
{
	D3D12ImageView::D3D12ImageView(const ImageViewDesc& desc, RawPtr<Image> image)
		: m_desc(desc), m_image(image)
	{
		// Keep a reference to the image, as it should be alive until all views have been destroyed.
		m_image->IncRef();

		m_format = image->GetFormat();
		m_imageUsage = image->GetUsage();
		m_imageAspect = image->GetImageAspect();
		m_isSwapchainImage = image->IsSwapchainImage();

		if (m_imageUsage == ImageUsage::Attachment)
		{
			VT_ENSURE(m_desc.viewType == ImageViewType::View2DArray || m_desc.viewType == ImageViewType::View2D);
			CreateRTVDSV();
		}
		else if (m_imageUsage == ImageUsage::AttachmentStorage)
		{
			CreateRTVDSV();
			CreateUAV();
		}
		else if (m_imageUsage == ImageUsage::Storage)
		{
			CreateUAV();
		}

		CreateSRV();
	}

	D3D12ImageView::~D3D12ImageView()
	{
		// Remove the reference we add on creation.
		m_image->DecRef();

		RHIModule::GetInstance().DestroyResource([srvDescriptor = m_srvDescriptor, uavDescriptor = m_uavDescriptor, rtvDsvDescriptor = m_rtvDsvDescriptor, viewUsage = m_viewUsage]()
		{
			if (srvDescriptor.IsValid())
			{
				g_descriptorManager.Free(D3D12DescriptorType::CBV_SRV_UAV, srvDescriptor);
			}

			if (uavDescriptor.IsValid())
			{
				g_descriptorManager.Free(D3D12DescriptorType::CBV_SRV_UAV, uavDescriptor);
			}
			
			if (rtvDsvDescriptor.IsValid())
			{
				if (EnumValueContainsFlag(viewUsage, D3D12ViewType::DSV))
				{
					g_descriptorManager.Free(D3D12DescriptorType::DSV, rtvDsvDescriptor);
				}
				else
				{
					g_descriptorManager.Free(D3D12DescriptorType::RTV, rtvDsvDescriptor);
				}
			}
		});
	}

	const PixelFormat D3D12ImageView::GetFormat() const
	{
		return m_format;
	}

	const ImageAspect D3D12ImageView::GetImageAspect() const
	{
		return m_imageAspect;
	}

	const uint64_t D3D12ImageView::GetDeviceAddress() const
	{
		return m_image->GetDeviceAddress();
	}

	const ImageUsage D3D12ImageView::GetImageUsage() const
	{
		return m_imageUsage;
	}

	const ImageViewType D3D12ImageView::GetViewType() const
	{
		return m_desc.viewType;
	}

	const bool D3D12ImageView::IsSwapchainView() const
	{
		return m_isSwapchainImage;
	}

	void* D3D12ImageView::GetHandleImpl() const
	{
		return m_image->GetHandle<ID3D12Resource*>();
	}

	const ImageViewDesc& D3D12ImageView::GetDesc() const
	{
		return m_desc;
	}

	RawPtr<Image> D3D12ImageView::GetImage() const
	{
		return m_image;
	}

	void D3D12ImageView::CreateRTVDSV()
	{
		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();

		const uint32_t numLayers = m_desc.layerCount == ImageViewDesc::LayerCountMax ? m_image->GetLayerCount() : m_desc.layerCount;

		if (Utility::IsDepthFormat(m_format))
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
			const auto d3d12Format = ConvertFormatToD3D12Format(m_format);
			const bool isTypeless = Utility::IsFormatTypeless(d3d12Format);

			if (isTypeless)
			{
				viewDesc.Format = Utility::GetDSVFormatFromTypeless(d3d12Format);
			}
			else
			{
				viewDesc.Format = d3d12Format;
			}


			if (m_desc.viewType == ImageViewType::View2D)
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = m_desc.baseMipLevel;
			}
			else if (m_desc.viewType == ImageViewType::View2DArray)
			{
				viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				viewDesc.Texture2DArray.ArraySize = numLayers;
				viewDesc.Texture2DArray.FirstArraySlice = m_desc.baseArrayLayer;
				viewDesc.Texture2DArray.MipSlice = m_desc.baseMipLevel;
			}

			m_viewUsage = D3D12ViewType::DSV;
			m_rtvDsvDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::DSV);

			d3d12Device->CreateDepthStencilView(m_image->GetHandle<ID3D12Resource*>(), &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_rtvDsvDescriptor.GetCPUPointer()));
		}
		else
		{
			D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};

			if (m_desc.viewType == ImageViewType::View2D)
			{
				viewDesc.Format = ConvertFormatToD3D12Format(m_format);
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = m_desc.baseMipLevel;
				viewDesc.Texture2D.PlaneSlice = m_desc.baseArrayLayer;
			}

			m_viewUsage = D3D12ViewType::RTV;
			m_rtvDsvDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::RTV);

			d3d12Device->CreateRenderTargetView(m_image->GetHandle<ID3D12Resource*>(), &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_rtvDsvDescriptor.GetCPUPointer()));
		}
	}

	void D3D12ImageView::CreateUAV()
	{
		if (m_desc.viewType == ImageViewType::ViewCube)
		{
			return;
		}

		D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};

		const auto d3d12Format = ConvertFormatToD3D12Format(m_format);
		const bool isTypeless = Utility::IsFormatTypeless(d3d12Format);

		const uint32_t numLayers = m_desc.layerCount == ImageViewDesc::LayerCountMax ? m_image->GetLayerCount() : m_desc.layerCount;

		if (isTypeless)
		{
			viewDesc.Format = Utility::GetSRVUAVFormatFromTypeless(d3d12Format);
		}
		else
		{
			viewDesc.Format = d3d12Format;
		}

		if (m_desc.viewType == ImageViewType::View1D)
		{
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			viewDesc.Texture1D.MipSlice = m_desc.baseMipLevel;
		}
		else if (m_desc.viewType == ImageViewType::View1DArray)
		{
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			viewDesc.Texture1DArray.ArraySize = numLayers;
			viewDesc.Texture1DArray.FirstArraySlice = m_desc.baseArrayLayer;
			viewDesc.Texture1DArray.MipSlice = m_desc.baseMipLevel;
		}
		else if (m_desc.viewType == ImageViewType::View2D)
		{
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = m_desc.baseMipLevel;
			viewDesc.Texture2D.PlaneSlice = 0;
		}
		else if (m_desc.viewType == ImageViewType::View2DArray)
		{
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.ArraySize = numLayers;
			viewDesc.Texture2DArray.FirstArraySlice = m_desc.baseArrayLayer;
			viewDesc.Texture2DArray.MipSlice = m_desc.baseMipLevel;
			viewDesc.Texture2DArray.PlaneSlice = 0;
		}
		else if (m_desc.viewType == ImageViewType::View3D)
		{
			viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			viewDesc.Texture3D.MipSlice = m_desc.baseMipLevel;
			viewDesc.Texture3D.FirstWSlice = m_desc.baseArrayLayer;
			viewDesc.Texture3D.WSize = numLayers;
		}

		m_viewUsage |= D3D12ViewType::UAV;
		m_uavDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::CBV_SRV_UAV);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		d3d12Device->CreateUnorderedAccessView(m_image->GetHandle<ID3D12Resource*>(), nullptr, &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_uavDescriptor.GetCPUPointer()));
	}

	void D3D12ImageView::CreateSRV()
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		const auto d3d12Format = ConvertFormatToD3D12Format(m_format);
		const bool isTypeless = Utility::IsFormatTypeless(d3d12Format);

		const uint32_t numMips = m_desc.mipCount == ImageViewDesc::MipCountMax ? m_image->GetMipCount() : m_desc.mipCount;
		const uint32_t numLayers = m_desc.layerCount == ImageViewDesc::LayerCountMax ? m_image->GetLayerCount() : m_desc.layerCount;

		if (isTypeless)
		{
			viewDesc.Format = Utility::GetSRVUAVFormatFromTypeless(d3d12Format);
		}
		else
		{
			viewDesc.Format = d3d12Format;
		}
		if (m_desc.viewType == ImageViewType::View1D)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			viewDesc.Texture1D.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.Texture1D.MipLevels = numMips;
			viewDesc.Texture1D.ResourceMinLODClamp = 0.f;
		}
		else if (m_desc.viewType == ImageViewType::View1DArray)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			viewDesc.Texture1DArray.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.Texture1DArray.MipLevels = numMips;
			viewDesc.Texture1DArray.ArraySize = numLayers;
			viewDesc.Texture1DArray.FirstArraySlice = m_desc.baseArrayLayer;
			viewDesc.Texture1DArray.ResourceMinLODClamp = 0.f;
		}
		else if (m_desc.viewType == ImageViewType::View2D)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.Texture2D.MipLevels = numMips;
			viewDesc.Texture2D.PlaneSlice = 0;
			viewDesc.Texture2D.ResourceMinLODClamp = 0.f;
		}
		else if (m_desc.viewType == ImageViewType::View2DArray)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			viewDesc.Texture2DArray.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.Texture2DArray.MipLevels = numMips;
			viewDesc.Texture2DArray.ArraySize = numLayers;
			viewDesc.Texture2DArray.FirstArraySlice = m_desc.baseArrayLayer;
			viewDesc.Texture2DArray.ResourceMinLODClamp = 0.f;
			viewDesc.Texture2DArray.PlaneSlice = 0;
		}
		else if (m_desc.viewType == ImageViewType::View3D)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			viewDesc.Texture3D.MipLevels = numMips;
			viewDesc.Texture3D.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.Texture3D.ResourceMinLODClamp = 0.f;
		}
		else if (m_desc.viewType == ImageViewType::ViewCube)
		{
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			viewDesc.TextureCube.MipLevels = numMips;
			viewDesc.TextureCube.MostDetailedMip = m_desc.baseMipLevel;
			viewDesc.TextureCube.ResourceMinLODClamp = 0.f;
		}

		m_viewUsage |= D3D12ViewType::SRV;
		m_srvDescriptor = g_descriptorManager.Allocate(D3D12DescriptorType::CBV_SRV_UAV);

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		d3d12Device->CreateShaderResourceView(m_image->GetHandle<ID3D12Resource*>(), &viewDesc, D3D12_CPU_DESCRIPTOR_HANDLE(m_srvDescriptor.GetCPUPointer()));
	}
}
