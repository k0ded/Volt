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

		virtual IntRef<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) = 0;
		virtual IntRef<RHI::Buffer> GetRHIBuffer() const = 0;
		virtual bool IsTransientlyAllocated() const = 0;
	};

	class RGRHITextureResource
	{
	public:
		virtual ~RGRHITextureResource() = default;

		virtual IntRef<RHI::ImageView> GetOrCreateView(const RHI::ImageViewDesc& desc) = 0;
		virtual IntRef<RHI::Image> GetRHITexture() const = 0;
		virtual bool IsTransientlyAllocated() const = 0;
	};

	class RGRHIUniformBufferResource
	{
	public:
		virtual ~RGRHIUniformBufferResource() = default;

		virtual IntRef<RHI::BufferView> GetOrCreateView(const RHI::BufferViewDesc& desc) = 0;
		virtual IntRef<RHI::UniformBuffer> GetRHIUniformBuffer() const = 0;
	};
}
