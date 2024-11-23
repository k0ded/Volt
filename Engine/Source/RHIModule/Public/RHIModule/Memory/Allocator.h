#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;
	class MemoryPool;

	class VTRHI_API Allocator : public RHIInterface
	{
	public:
		virtual ~Allocator() = default;

		virtual Handle<Allocation> CreateBuffer(const uint64_t size, BufferUsage usage, MemoryUsage memoryUsage, const std::string& name) = 0;
		virtual Handle<Allocation> CreateImage(const ImageSpecification& imageSpecification, MemoryUsage memoryUsage) = 0;

		virtual void DestroyBuffer(Handle<Allocation> allocation) = 0;
		virtual void DestroyImage(Handle<Allocation> allocation) = 0;

		// Note: This is slow operation!
		virtual Vector<Handle<Allocation>> GetActiveImageAllocations() const = 0;

		// Note: This is slow operation!
		virtual Vector<Handle<Allocation>> GetActiveBufferAllocations() const = 0;

		// Used for allocation cache
		virtual void Update() = 0;

	protected:
		Allocator() = default;
	};

	class VTRHI_API DefaultAllocator : public Allocator
	{
	public:
		virtual ~DefaultAllocator() override = default;

		static RefPtr<DefaultAllocator> Create();

	protected:
		DefaultAllocator() = default;
	};

	class VTRHI_API TransientAllocator : public Allocator
	{
	public:
		virtual ~TransientAllocator() override = default;

		static RefPtr<TransientAllocator> Create();

	protected:
		TransientAllocator() = default;
	};
}
