#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Memory/Allocation.h"

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	struct AllocationContainer
	{
		Handle<Allocation> allocation;
		size_t framesAlive = 0;
	};
	
	struct AllocationsToRemove
	{
		Vector<Handle<Allocation>> imageAllocations;
		Vector<Handle<Allocation>> bufferAllocations;
	};

	class VTRHI_API AllocationCache
	{
	public:
		Handle<Allocation> TryGetImageAllocationFromHash(const size_t hash);
		Handle<Allocation> TryGetBufferAllocationFromHash(const size_t hash);
		void QueueImageAllocationForRemoval(Handle<Allocation> alloc);
		void QueueBufferAllocationForRemoval(Handle<Allocation> alloc);
		AllocationsToRemove UpdateAndGetAllocationsToDestroy();

		inline const auto& GetImageAllocations() const { return m_imageAllocations; }
		inline const auto& GetBufferAllocations() const { return m_bufferAllocations; }

	private:
		std::mutex m_imageAllocationMutex;
		std::mutex m_bufferAllocationMutex;

		Vector<AllocationContainer> m_imageAllocations;
		Vector<AllocationContainer> m_bufferAllocations;
	};
}
