#include "rcpch.h"
#include "RenderCore/RenderGraph/GPUReadbackBuffer.h"

#include <RHIModule/Buffers/StorageBuffer.h>

namespace Volt
{
	GPUReadbackBuffer::GPUReadbackBuffer(size_t size)
	{
		RHI::BufferDesc desc{};
		desc.count = 1;
		desc.elementSize = size;
		desc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferDst;
		desc.memoryUsage = RHI::MemoryUsage::GPUToCPU;
		desc.debugName = "GPU Readback Buffer";

		m_buffer = RHI::StorageBuffer::Create(desc);
	}
}
