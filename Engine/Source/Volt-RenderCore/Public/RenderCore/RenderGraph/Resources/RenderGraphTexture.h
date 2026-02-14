#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Images/ImageView.h>

namespace Volt
{
	class RGRHITextureResource;

	struct RGTextureSubResourceRange
	{
		uint32_t baseMipLevel : 8;
		uint32_t baseArrayLayer : 8;
		uint32_t mipCount : 8;
		uint32_t layerCount : 8;
	};

	struct RGTextureDesc : public RHI::ImageDesc
	{
		template<RHI::PixelFormat PixelFormat>
		static RGTextureDesc Create2D(const uint32_t width, const uint32_t height, RHI::ImageUsage usage, const std::string& name = "Texture")
		{
			VT_ASSERT_MSG(width > 0 && height > 0, "Width and height must not be zero!");
			
			RGTextureDesc resultDesc{};
			resultDesc.width = width;
			resultDesc.height = height;
			resultDesc.usage = usage;
			resultDesc.debugName = name;
			resultDesc.imageType = RHI::ResourceType::Image2D;
			resultDesc.format = PixelFormat;

			return resultDesc;
		}

		template<RHI::PixelFormat PixelFormat>
		static RGTextureDesc CreateCube(const uint32_t width, const uint32_t height, RHI::ImageUsage usage, const std::string& name = "Texture")
		{
			VT_ASSERT_MSG(width > 0 && height > 0, "Width and height must not be zero!");
			
			RGTextureDesc resultDesc{};
			resultDesc.width = width;
			resultDesc.height = height;
			resultDesc.usage = usage;
			resultDesc.debugName = name;
			resultDesc.imageType = RHI::ResourceType::Image2D;
			resultDesc.format = PixelFormat;
			resultDesc.layers = 6;
			resultDesc.isCubeMap = true;

			return resultDesc;
		}
	};

	using RGTextureResourceAccessState = Vector<RGResourceAccessState>;

	class VTRC_API RGTexture : public RGResource
	{
	public:
		RGTexture(const RGTextureDesc& desc);
		~RGTexture() override = default;
		RGResourceType GetResourceType() const override;

		VT_INLINE void AssignRHIResource(RGRHITextureResource* resource) { m_rhiResource = resource; }

		VT_NODISCARD VT_INLINE const RGTextureDesc& GetDesc() const { return m_desc; }
		VT_NODISCARD VT_INLINE RGRHITextureResource* GetRHIResource() const { return m_rhiResource; }

		RGTextureSubResourceState firstAccess;
		RGTextureResourceAccessState lastAccess;

	private:
		void InitializeSubResources();

		RGTextureDesc m_desc;
		RGRHITextureResource* m_rhiResource = nullptr;
	};

	using RGTextureRef = RGTexture*;

	struct RGTextureSRVDesc
	{ 
		RGTextureRef textureResource;

		uint32_t baseMipLevel = 0;
		uint32_t baseArrayLayer = 0;
		uint32_t mipCount = RHI::ImageViewDesc::MipCountMax;
		uint32_t layerCount = RHI::ImageViewDesc::LayerCountMax;
	};

	class VTRC_API RGTextureSRV : public RGResourceSRV
	{
	public:
		RGTextureSRV(const RGTextureSRVDesc& desc);
		~RGTextureSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.textureResource; }
		VT_NODISCARD VT_INLINE RGTextureSubResourceRange GetSubResourceRange() const
		{
			return
			{
				.baseMipLevel = m_desc.baseMipLevel,
				.baseArrayLayer = m_desc.baseArrayLayer,
				.mipCount = m_desc.mipCount,
				.layerCount = m_desc.layerCount
			};
		}

		VT_NODISCARD VT_INLINE const RGTextureSRVDesc& GetDesc() const { return m_desc; }
		VT_INLINE void AssignRHIView(RefPtr<RHI::ImageView> view) { m_rhiView = view; }
		VT_INLINE RefPtr<RHI::ImageView> GetRHIView() { return m_rhiView; }

	private:
		RefPtr<RHI::ImageView> m_rhiView;
		RGTextureSRVDesc m_desc;
	};

	struct RGTextureUAVDesc
	{
		RGTextureRef textureResource;

		uint32_t baseMipLevel = 0;
		uint32_t baseArrayLayer = 0;
		uint32_t mipCount = RHI::ImageViewDesc::MipCountMax;
		uint32_t layerCount = RHI::ImageViewDesc::LayerCountMax;
	};

	class VTRC_API RGTextureUAV : public RGResourceUAV
	{
	public:
		RGTextureUAV(const RGTextureUAVDesc& desc);
		~RGTextureUAV() override = default;

		RGResourceRef GetResource() const override { return m_desc.textureResource; }
		VT_NODISCARD VT_INLINE RGTextureSubResourceRange GetSubResourceRange() const 
		{ 
			return 
			{
				.baseMipLevel = m_desc.baseMipLevel,
				.baseArrayLayer = m_desc.baseArrayLayer,
				.mipCount = m_desc.mipCount,
				.layerCount = m_desc.layerCount
			};
		}

		VT_NODISCARD VT_INLINE const RGTextureUAVDesc& GetDesc() const { return m_desc; }
		VT_INLINE void AssignRHIView(RefPtr<RHI::ImageView> view) { m_rhiView = view; }
		VT_INLINE RefPtr<RHI::ImageView> GetRHIView() { return m_rhiView; }

	private:
		RefPtr<RHI::ImageView> m_rhiView;
		RGTextureUAVDesc m_desc;
	};
}
