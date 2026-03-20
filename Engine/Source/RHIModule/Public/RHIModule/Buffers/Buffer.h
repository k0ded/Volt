#pragma once

#include "RHIModule/Core/RHIResource.h"
#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/Buffers/BufferView.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Pointers/IntRef.h>

namespace Volt::RHI
{
	class CommandBuffer;
	class Allocation;

	class Buffer : public RHIResource
	{
	public:
		~Buffer() override = default;

		virtual const BufferDesc& GetDesc() const = 0;
		virtual uint64_t GetElementSize() const = 0;
		virtual uint64_t GetNumElements() const = 0;
		virtual IntRef<BufferView> GetView(const BufferViewDesc& desc = {}) = 0;
		virtual void Unmap() = 0;
		 
		template<typename T>
		T* Map();

		VTRHI_API static IntRef<Buffer> Create(const BufferDesc& desc);

	protected:
		virtual void* MapInternal() = 0;

		Buffer() = default;
	};

	template<typename T>
	inline T* Buffer::Map()
	{
		return reinterpret_cast<T*>(MapInternal());
	}
}
