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

	struct TransientHeapCreateInfo
	{
		uint64_t pageSize = 0;
		uint32_t alignment = 0;
		TransientHeapFlags flags = TransientHeapFlags::AllowAll;
	};

	class VTRHI_API TransientHeap : public RHIInterface
	{
	public:
		virtual void ReservePages(uint32_t numPages) = 0;

		static RefPtr<TransientHeap> Create(const TransientHeapCreateInfo& createInfo);
	};
}
