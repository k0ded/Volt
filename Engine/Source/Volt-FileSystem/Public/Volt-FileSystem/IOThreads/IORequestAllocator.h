#pragma once

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt
{
	class IORequest;

	class IORequestAllocator
	{
	public:
		inline static constexpr size_t MaxIORequestSize = 512;
		inline static constexpr size_t PageSize = 128;

		template<typename RequestType, typename... Args>
		RequestType* Allocate(Args&&... args);

		void Free(IORequest* request);

	private:
		struct IORequestMemory
		{
			uint8_t bytes[MaxIORequestSize];
		};

		PagedAtomicArenaAllocator<IORequestMemory, PageSize> m_allocator;
	};

	template<typename RequestType, typename... Args>
	RequestType* IORequestAllocator::Allocate(Args&&... args)
	{
		static_assert(sizeof(RequestType) < MaxIORequestSize);

		IORequestMemory* memory = m_allocator.Allocate();
		RequestType* request = new(memory) RequestType(std::forward<Args>(args)...);

		return request;
	}
}
