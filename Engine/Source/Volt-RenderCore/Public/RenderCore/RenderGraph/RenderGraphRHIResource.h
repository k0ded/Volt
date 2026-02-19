#pragma once

#include <RHIModule/Buffers/BufferView.h>
#include <RHIModule/Buffers/Buffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>

#include <RHIModule/Images/Image.h>
#include <RHIModule/Images/ImageView.h>

namespace Volt
{
	class RGRHIBufferResource
	{
	public:
		virtual ~RGRHIBufferResource() = default;

		virtual RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) = 0;
		virtual RefPtr<RHI::Buffer> GetRHIBuffer() const = 0;
	};

	class RGRHITextureResource
	{
	public:
		virtual ~RGRHITextureResource() = default;

		virtual RefPtr<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc) = 0;
		virtual RefPtr<RHI::Image> GetRHITexture() const = 0;
	};

	class RGRHIUniformBufferResource
	{
	public:
		virtual ~RGRHIUniformBufferResource() = default;

		virtual RefPtr<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) = 0;
		virtual RefPtr<RHI::UniformBuffer> GetRHIUniformBuffer() const = 0;
	};
}
