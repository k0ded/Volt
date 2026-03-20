#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include "RHIModule/Buffers/BufferDesc.h"

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;
	class MemoryPool;

	struct BufferDesc;

	class VTRHI_API GPUAllocator : public RHIInterface
	{
	public:
		virtual ~GPUAllocator() = default;

		virtual Handle<Allocation> CreateBuffer(const BufferDesc& desc) = 0;
		virtual Handle<Allocation> CreateImage(const ImageDesc& imageSpecification, MemoryUsage memoryUsage) = 0;

		virtual void DestroyBuffer(Handle<Allocation> allocation) = 0;
		virtual void DestroyImage(Handle<Allocation> allocation) = 0;

		// Note: This is slow operation!
		virtual Vector<Handle<Allocation>> GetActiveImageAllocations() const = 0;

		// Note: This is slow operation!
		virtual Vector<Handle<Allocation>> GetActiveBufferAllocations() const = 0;

		// Used for allocation cache
		virtual void Update() = 0;

	protected:
		GPUAllocator() = default;
	};

	class VTRHI_API DefaultGPUAllocator : public GPUAllocator
	{
	public:
		virtual ~DefaultGPUAllocator() override = default;

		static IntRef<DefaultGPUAllocator> Create();

	protected:
		DefaultGPUAllocator() = default;
	};
}
