#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Buffers/Buffer.h"
#include "RHIModule/Memory/TransientHeap.h"

/*
* This is a specialization of Buffer, which is supposed to be used for frame transient buffers.
* It does not own it's memory, but is instead assigned it.
* It does own it's image handle.
*/

namespace Volt::RHI
{
	class TransientBuffer : public Buffer
	{
	public:
		~TransientBuffer() override = default;

		virtual void BindMemory(IntRef<RHI::TransientHeap> heap, uint32_t pageIndex, uint64_t offset) = 0;

		VTRHI_API static IntRef<TransientBuffer> Create(const BufferDesc& desc);

	protected:
		TransientBuffer() = default;
	};
}
