#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Buffers/BufferDesc.h>

namespace Volt
{
	struct RGBufferDesc : public RHI::BufferDesc
	{
		bool isTexelBufferDesc = false;

		template<typename T, typename CountType>
		static RGBufferDesc CreateStructuredBufferDesc(const CountType count, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(count);
			desc.elementSize = sizeof(T);
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.debugName = name;

			return desc;
		}

		template<typename T, typename CountType>
		static RGBufferDesc CreateBufferDesc(const CountType count, const std::string& name = "Buffer", const RHI::MemoryUsage usage = RHI::MemoryUsage::GPU)
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(count);
			desc.elementSize = sizeof(T);
			desc.usage = RHI::BufferUsage::TexelBuffer;
			desc.memoryUsage = usage;
			desc.debugName = name;
			desc.isTexelBufferDesc = true;

			return desc;
		}

		template<typename T, typename CountType>
		static RGBufferDesc CreateMappableBufferDesc(const CountType count, const RHI::BufferUsage usage, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(count);
			desc.elementSize = sizeof(T);
			desc.usage = usage;
			desc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			desc.debugName = name;
		
			return desc;
		}

		template<typename T, typename CountType>
		static RGBufferDesc CreateBufferDescGPU(const CountType count, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(count > 0, "Count must not be zero!");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(count);
			desc.elementSize = sizeof(T);
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.debugName = name;

			return desc;
		}

		template<typename SizeType>
		static RGBufferDesc CreateStagingDesc(const SizeType size, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(size > 0, "Size must not be zero!");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(count);
			desc.elementSize = static_cast<uint64_t>(size);
			desc.usage = RHI::BufferUsage::TransferSrc | RHI::BufferUsage::StorageBuffer;
			desc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			desc.debugName = name;

			return desc;
		}
	};

	class VTRC_API RGBuffer : public RGResource
	{
	public:
		RGBuffer(const RGBufferDesc& desc);
		~RGBuffer() override = default;
		RGResourceType GetResourceType() const override;

		bool HasProducer(RGResourceUAV* uav) const override;
		bool HasProducer() const override;
		void AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav) override;
		void AddProducer(Handle<RenderGraphPass> pass) override;

		VT_NODISCARD VT_INLINE const RGBufferDesc& GetDesc() const { return m_desc; }

	private:
		bool m_isProduced = false;
		RGBufferDesc m_desc;
	};

	using RGBufferRef = RGBuffer*;

	struct RGBufferSRVDesc
	{
		RGBufferRef bufferResource;

		// Used for texel buffers
		RHI::PixelFormat format = RHI::PixelFormat::UNDEFINED;
	};

	class VTRC_API RGBufferSRV : public RGResourceSRV
	{
	public:
		RGBufferSRV(const RGBufferSRVDesc& desc);
		~RGBufferSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }
		const RGBufferSRVDesc& GetDesc() const { return m_desc; }

	private:
		RGBufferSRVDesc m_desc;
	};

	struct RGBufferUAVDesc
	{ 
		RGBufferRef bufferResource;

		// Used for texel buffers
		RHI::PixelFormat format = RHI::PixelFormat::UNDEFINED;
	};

	class VTRC_API RGBufferUAV : public RGResourceUAV
	{
	public:
		RGBufferUAV(const RGBufferUAVDesc& desc);
		~RGBufferUAV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }
		const RGBufferUAVDesc& GetDesc() const { return m_desc; }
	
	private:
		RGBufferUAVDesc m_desc;
	};
}
