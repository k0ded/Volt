#pragma once

#include <CoreUtilities/CompilerTraits.h>

#include <cstdint>
#include <limits>

namespace Volt::RHI
{
	class BindlessIndex
	{
	public:
		inline static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

		BindlessIndex() noexcept;
		explicit BindlessIndex(uint32_t index) noexcept;

		VT_NODISCARD VT_INLINE uint32_t Get() const { return m_index; }
		VT_NODISCARD VT_INLINE bool IsValid() const { return m_index != InvalidIndex; }

	private:
		uint32_t m_index;
	};

	inline BindlessIndex::BindlessIndex() noexcept
		: m_index(InvalidIndex)
	{}

	inline BindlessIndex::BindlessIndex(uint32_t index) noexcept
		: m_index(index)
	{}
}
