#pragma once

#include "RHIModule/Core/RHIResource.h"
#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/Buffers/BufferView.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt::RHI
{
	class CommandBuffer;
	class Allocation;

	class VTRHI_API StorageBuffer : public RHIResource
	{
	public:
		~StorageBuffer() override = default;

		virtual void Resize(const uint64_t size) = 0;
		virtual void ResizeWithCount(const uint32_t count) = 0;
		virtual const BufferDesc& GetDesc() const = 0;

		virtual const uint64_t GetElementSize() const = 0;
		virtual const uint32_t GetCount() const = 0;
		virtual Handle<Allocation> GetAllocation() const = 0;

		virtual void Unmap() = 0;
		virtual void SetData(const void* data, const size_t size) = 0;
		virtual void SetData(RefPtr<CommandBuffer> commandBuffer, const void* data, const size_t size) = 0;

		virtual RefPtr<BufferView> GetView(const BufferViewDesc& desc = {}) = 0;
		 
		template<typename T>
		T* Map();

		static RefPtr<StorageBuffer> Create(const BufferDesc& desc, RefPtr<GPUAllocator> allocator = nullptr);

	protected:
		virtual void* MapInternal() = 0;

		StorageBuffer() = default;
	};

	template<typename T>
	inline T* StorageBuffer::Map()
	{
		return reinterpret_cast<T*>(MapInternal());
	}
}
