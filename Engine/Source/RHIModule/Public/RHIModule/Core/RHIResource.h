
#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/ResourceStateTracker.h"

#include <string_view>

namespace Volt::RHI
{
	class RHIResource : public ArenaRHIInterface
	{
	public:
		~RHIResource() override = default;
		VT_DELETE_COPY_MOVE(RHIResource);

		virtual constexpr ResourceType GetType() const = 0;
		virtual void SetName(const std::string& name) = 0;
		virtual std::string_view GetName() const = 0;
		virtual uint64_t GetDeviceAddress() const = 0;
		virtual const MemoryRequirement& GetMemoryRequirements() const = 0;
		virtual uint64_t GetResourceByteSize() const = 0;

		VT_INLINE const ResourceStateTracker& GetResourceStateTracker() const { return m_resourceStateTracker; }
		VT_INLINE ResourceStateTracker& GetResourceStateTrackerMutable() { return m_resourceStateTracker; }

	protected:
		RHIResource() = default;

		ResourceStateTracker m_resourceStateTracker;
	};
}
