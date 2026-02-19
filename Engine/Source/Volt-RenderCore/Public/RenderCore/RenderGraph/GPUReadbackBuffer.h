#pragma once

#include <CoreUtilities/Pointers/RefPtr.h>
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

		VT_NODISCARD VT_INLINE RefPtr<RHI::Buffer> GetBuffer() const { return m_buffer; }
		VT_NODISCARD VT_INLINE bool IsReady() const { return m_isReady.load(); }

	private:
		friend class RenderGraph;

		std::atomic_bool m_isReady = false;
		RefPtr<RHI::Buffer> m_buffer;
	};
}
