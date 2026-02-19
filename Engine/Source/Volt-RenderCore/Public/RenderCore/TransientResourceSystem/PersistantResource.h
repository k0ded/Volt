#pragma once

#include "RenderCore/RenderGraph/RenderGraphRHIResource.h"
#include "RenderCore/TransientResourceSystem/ResourceViewCache.h"

namespace Volt
{
	class PersistantBufferResource : public RGRHIBufferResource
	{
	public:
		PersistantBufferResource(RefPtr<RHI::Buffer> buffer);
		~PersistantBufferResource() override = default;

		VT_INLINE RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE RefPtr<RHI::Buffer> GetRHIBuffer() const override { return m_buffer; }

	private:
		TransientBufferViewCache m_viewCache;
		RefPtr<RHI::Buffer> m_buffer;
	};

	class PersistantTextureResource : public RGRHITextureResource
	{
	public:
		PersistantTextureResource(RefPtr<RHI::Image> image);
		~PersistantTextureResource() override = default;

		VT_INLINE RefPtr<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE RefPtr<RHI::Image> GetRHITexture() const override { return m_image; }

	private:
		TransientImageViewCache m_viewCache;
		RefPtr<RHI::Image> m_image;
	};

	class PersistantUniformBufferResource : public RGRHIUniformBufferResource
	{
	public:
		PersistantUniformBufferResource(RefPtr<RHI::UniformBuffer> uniformBuffer);
		~PersistantUniformBufferResource() override = default;

		VT_INLINE RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE RefPtr<RHI::UniformBuffer> GetRHIUniformBuffer() const override { return m_uniformBuffer; }

	private:
		TransientUniformBufferViewCache m_viewCache;
		RefPtr<RHI::UniformBuffer> m_uniformBuffer;
	};
}
