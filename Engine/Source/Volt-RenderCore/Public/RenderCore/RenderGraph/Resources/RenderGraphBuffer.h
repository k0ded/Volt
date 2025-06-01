#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>

namespace Volt
{
	struct RGBufferDesc
	{
		uint32_t count;
		uint64_t elementSize;
		RHI::BufferUsage usage = RHI::BufferUsage::None;
		RHI::MemoryUsage memoryUsage = RHI::MemoryUsage::GPU;

		std::string name;
	
		template<typename T, typename CountType>
		static RGBufferDesc CreateBufferDesc(const CountType count, const RHI::BufferUsage usage, const RHI::MemoryUsage memoryUsage, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");
			return { .count = static_cast<uint32_t>(count), .elementSize = sizeof(T), .usage = usage, .memoryUsage = memoryUsage, .name = name };
		}

		template<typename T, typename CountType>
		static RGBufferDesc CreateMappableBufferDesc(const CountType count, const RHI::BufferUsage usage, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");
			return { .count = static_cast<uint32_t>(count), .elementSize = sizeof(T), .usage = usage, .memoryUsage = RHI::MemoryUsage::CPUToGPU, .name = name };
		}

		template<typename T, typename CountType>
		static RGBufferDesc CreateBufferDescGPU(const CountType count, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");
			return { .count = static_cast<uint32_t>(count), .elementSize = sizeof(T), .usage = RHI::BufferUsage::StorageBuffer, .memoryUsage = RHI::MemoryUsage::GPU, .name = name };
		}

		template<typename SizeType>
		static RGBufferDesc CreateStagingDesc(const SizeType size, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(size > 0, "Size must not be zero!");
			return { .count = 1, .elementSize = static_cast<uint64_t>(size), .usage = RHI::BufferUsage::TransferSrc | RHI::BufferUsage::StorageBuffer, .memoryUsage = RHI::MemoryUsage::CPUToGPU, .name = name };
		}
	};

	class VTRC_API RGBuffer : public RGResource
	{
	public:
		RGBuffer(const RGBufferDesc& desc);
		~RGBuffer() override = default;
		RGResourceType GetResourceType() const override;

		VT_NODISCARD VT_INLINE const RGBufferDesc& GetDesc() const { return m_desc; }

	private:
		RGBufferDesc m_desc;
	};

	using RGBufferRef = RGBuffer*;

	struct RGBufferSRVDesc
	{
		RGBufferRef bufferResource;
	};

	class VTRC_API RGBufferSRV : public RGResourceSRV
	{
	public:
		RGBufferSRV(const RGBufferSRVDesc& desc);
		~RGBufferSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }

	private:
		RGBufferSRVDesc m_desc;
	};

	struct RGBufferUAVDesc
	{ 
		RGBufferRef bufferResource;
	};

	class VTRC_API RGBufferUAV : public RGResourceUAV
	{
	public:
		RGBufferUAV(const RGBufferUAVDesc& desc);
		~RGBufferUAV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }
	
	private:
		RGBufferUAVDesc m_desc;
	};
}
