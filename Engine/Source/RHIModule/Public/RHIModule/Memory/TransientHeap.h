#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/RHIInterface.h"

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/Allocators/Handle.h>

namespace Volt::RHI
{
	class Allocation;

	enum class TransientHeapFlags : uint8_t
	{
		None = 0,
		AllowBuffers = BIT(0),
		AllowTextures = BIT(1),
		AllowRenderTargets = BIT(2),
		AllowMappable = BIT(3),

		AllowAll = AllowBuffers | AllowTextures | AllowRenderTargets
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(TransientHeapFlags);

	struct TransientBufferCreateInfo
	{
		uint64_t size = 0;
		size_t hash = 0;
		BufferUsage usage = BufferUsage::None;
		MemoryUsage memoryUsage = MemoryUsage::None;
	};

	struct TransientHeapCreateInfo
	{
		uint64_t pageSize = 0;
		uint32_t alignment = 0;
		TransientHeapFlags flags = TransientHeapFlags::AllowAll;
	};

	struct TransientImageCreateInfo
	{
		ImageDesc imageSpecification;
		uint64_t size = 0;
		size_t hash = 0;
	};

	class VTRHI_API TransientHeap : public RHIInterface
	{
	public:
		virtual Handle<Allocation> CreateBuffer(const TransientBufferCreateInfo& createInfo, const std::string& name) = 0;
		virtual Handle<Allocation> CreateImage(const TransientImageCreateInfo& createInfo, const std::string& name) = 0;

		virtual void ForfeitBuffer(Handle<Allocation> allocation) = 0;
		virtual void ForfeitImage(Handle<Allocation> allocation) = 0;

		virtual const bool IsAllocationSupported(const uint64_t size, TransientHeapFlags heapFlags) const = 0;
		virtual const UUID64 GetHeapID() const = 0;

		static RefPtr<TransientHeap> Create(const TransientHeapCreateInfo& createInfo);
	};
}
