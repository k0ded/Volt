#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Buffers/Buffer.h"

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

		// #TODO_Ivar: Implement memory assigning functions.

		VTRHI_API static RefPtr<TransientBuffer> Create(const BufferDesc& desc);

	protected:
		TransientBuffer() = default;
	};
}
