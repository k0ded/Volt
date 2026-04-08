#pragma once

#include <RHIModule/Buffers/Buffer.h>

#include <CoreUtilities/Pointers/IntRef.h>
#include <CoreUtilities/Pointers/RawPtr.h>

namespace Volt
{
	namespace RHI
	{
		class Buffer;
	}

	class GPUReadbackBuffer
	{
	public:
		GPUReadbackBuffer(size_t size);

		VT_NODISCARD VT_INLINE IntRef<RHI::Buffer> GetBuffer() const { return m_buffer; }
		VT_NODISCARD VT_INLINE bool IsReady() const { return m_isReady.load(); }

	private:
		friend class RenderGraph;

		std::atomic_bool m_isReady = false;
		IntRef<RHI::Buffer> m_buffer;
	};
}
