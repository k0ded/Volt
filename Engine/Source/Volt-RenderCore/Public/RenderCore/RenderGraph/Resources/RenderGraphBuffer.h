#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Buffers/BufferDesc.h>
#include <RHIModule/Buffers/BufferView.h>

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

		template<typename SizeType>
		static RGBufferDesc CreateByteAddressDesc(const SizeType byteSize, const std::string& name = "Buffer")
		{
			VT_ASSERT_MSG(byteSize > 0, "Size must not be zero!");
			VT_ASSERT_MSG(byteSize % 4 == 0, "Size must be 4 byte aligned");

			RGBufferDesc desc{};
			desc.count = static_cast<uint32_t>(byteSize);
			desc.elementSize = 1u;
			desc.usage = RHI::BufferUsage::StorageBuffer;
			desc.memoryUsage = RHI::MemoryUsage::GPU;
			desc.debugName = name;

			return desc;
		}

		template<typename CommandType, typename SizeType>
		static RGBufferDesc CreateIndirectDesc(const SizeType numCommands, const std::string& name = "Buffer", const RHI::MemoryUsage memoryUsage = RHI::MemoryUsage::GPU)
		{
			VT_ASSERT_MSG(numCommands > 0, "Num commands must not be zero!");

			RGBufferDesc desc{};
			desc.count = numCommands;
			desc.elementSize = CommandType::SizeInUInts * sizeof(uint32_t);
			desc.usage = RHI::BufferUsage::IndirectBuffer | RHI::BufferUsage::TexelBuffer;
			desc.memoryUsage = memoryUsage;
			desc.debugName = name;
			desc.isTexelBufferDesc = true;

			return desc;
		}
	};

	class RGRHIBufferResource;

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

		VT_INLINE void AssignRHIResource(RGRHIBufferResource* resource) { m_rhiResource = resource; }

		VT_NODISCARD VT_INLINE const RGBufferDesc& GetDesc() const { return m_desc; }
		VT_NODISCARD VT_INLINE RGRHIBufferResource* GetRHIResource() const { return m_rhiResource; }

	private:
		bool m_isProduced = false;
		RGBufferDesc m_desc;

		RGRHIBufferResource* m_rhiResource;
	};

	using RGBufferRef = RGBuffer*;

	struct RGBufferSRVDesc
	{
		RGBufferRef bufferResource;

		uint64_t size = std::numeric_limits<uint64_t>::max();
		uint64_t offset = 0;

		// Used for texel buffers
		RHI::PixelFormat format = RHI::PixelFormat::UNDEFINED;

		static RGBufferSRVDesc Create(RGBufferRef buffer, uint64_t size)
		{
			return RGBufferSRVDesc{
				.bufferResource = buffer,
				.size = size
			};
		}

		static RGBufferSRVDesc Create(RGBufferRef buffer, uint64_t size, uint64_t offset)
		{
			return RGBufferSRVDesc{
				.bufferResource = buffer,
				.size = size,
				.offset = offset
			};
		}

		template<RHI::PixelFormat Format>
		static RGBufferSRVDesc Create(RGBufferRef buffer, uint64_t size, uint64_t offset)
		{
			return RGBufferSRVDesc{
				.bufferResource = buffer,
				.size = size,
				.offset = offset,
				.format = Format
			};
		}
	};

	class VTRC_API RGBufferSRV : public RGResourceSRV
	{
	public:
		RGBufferSRV(const RGBufferSRVDesc& desc);
		~RGBufferSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }
		const RGBufferSRVDesc& GetDesc() const { return m_desc; }

		VT_INLINE void AssignRHIView(RefPtr<RHI::BufferView> view) { m_rhiView = view; }
		VT_INLINE RefPtr<RHI::BufferView> GetRHIView() { return m_rhiView; }

	private:
		RefPtr<RHI::BufferView> m_rhiView;
		RGBufferSRVDesc m_desc;
	};

	struct RGBufferUAVDesc
	{ 
		RGBufferRef bufferResource;

		uint64_t size = std::numeric_limits<uint64_t>::max();
		uint64_t offset = 0;

		// Used for texel buffers
		RHI::PixelFormat format = RHI::PixelFormat::UNDEFINED;

		static RGBufferUAVDesc Create(RGBufferRef buffer, uint64_t size)
		{
			return RGBufferUAVDesc {
				.bufferResource = buffer,
				.size = size
			};
		}

		static RGBufferUAVDesc Create(RGBufferRef buffer, uint64_t size, uint64_t offset)
		{
			return RGBufferUAVDesc{
				.bufferResource = buffer,
				.size = size,
				.offset = offset
			};
		}

		template<RHI::PixelFormat Format>
		static RGBufferUAVDesc Create(RGBufferRef buffer, uint64_t size, uint64_t offset)
		{
			return RGBufferUAVDesc{
				.bufferResource = buffer,
				.size = size,
				.offset = offset,
				.format = Format
			};
		}
	};

	class VTRC_API RGBufferUAV : public RGResourceUAV
	{
	public:
		RGBufferUAV(const RGBufferUAVDesc& desc);
		~RGBufferUAV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }
		const RGBufferUAVDesc& GetDesc() const { return m_desc; }
	
		VT_INLINE void AssignRHIView(RefPtr<RHI::BufferView> view) { m_rhiView = view; }
		VT_INLINE RefPtr<RHI::BufferView> GetRHIView() { return m_rhiView; }

	private:
		RefPtr<RHI::BufferView> m_rhiView;
		RGBufferUAVDesc m_desc;
	};
}
