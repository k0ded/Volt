#pragma once

#include "RenderCore/RenderGraph/RenderGraphRHIResource.h"
#include "RenderCore/TransientResourceSystem/ResourceViewCache.h"

namespace Volt
{
	class PersistantBufferResource : public RGRHIBufferResource
	{
	public:
		PersistantBufferResource(IntRef<RHI::Buffer> buffer);
		~PersistantBufferResource() override;

		VT_INLINE IntRef<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE IntRef<RHI::Buffer> GetRHIBuffer() const override { return m_buffer; }
		VT_INLINE bool IsTransientlyAllocated() const override { return false; }

	private:
		TransientBufferViewCache m_viewCache;
		IntRef<RHI::Buffer> m_buffer;
	};

	class PersistantTextureResource : public RGRHITextureResource
	{
	public:
		PersistantTextureResource(IntRef<RHI::Image> image);
		~PersistantTextureResource() override = default;

		VT_INLINE IntRef<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE IntRef<RHI::Image> GetRHITexture() const override { return m_image; }
		VT_INLINE bool IsTransientlyAllocated() const override { return false; }

	private:
		TransientImageViewCache m_viewCache;
		IntRef<RHI::Image> m_image;
	};

	class PersistantUniformBufferResource : public RGRHIUniformBufferResource
	{
	public:
		PersistantUniformBufferResource(IntRef<RHI::UniformBuffer> uniformBuffer);
		~PersistantUniformBufferResource() override = default;

		VT_INLINE IntRef<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) override { return m_viewCache.GetOrCreateView(desc); }
		VT_INLINE IntRef<RHI::UniformBuffer> GetRHIUniformBuffer() const override { return m_uniformBuffer; }

	private:
		TransientUniformBufferViewCache m_viewCache;
		IntRef<RHI::UniformBuffer> m_uniformBuffer;
	};
}
