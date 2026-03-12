#include "rcpch.h"
#include "RenderCore/RenderGraph/GPUReadbackBuffer.h"

#include <RHIModule/Buffers/Buffer.h>

namespace Volt
{
	GPUReadbackBuffer::GPUReadbackBuffer(size_t size)
	{
		RHI::BufferDesc desc{};
		desc.numElements = 1;
		desc.elementSize = size;
		desc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferDst;
		desc.memoryUsage = RHI::MemoryUsage::GPUToCPU;
		desc.debugName = "GPU Readback Buffer";

		m_buffer = RHI::Buffer::Create(desc);
	}
}
