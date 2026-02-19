#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Buffers/Buffer.h"

namespace Volt::RHI::BufferUtility
{
	VTRHI_API extern void StagedBufferUpload(RefPtr<RHI::Buffer> buffer, const void* data, uint64_t size);
	VTRHI_API extern RefPtr<RHI::Buffer> ResizeBufferIfRequired(RefPtr<RHI::Buffer> buffer, uint64_t numElements);
}
