#pragma once

#include "VulkanRHIModule/LastSubmissionTracker.h"

#include <RHIModule/Core/RHIInterface.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Variant.h>

#include <unordered_set>

struct VkSemaphore_T;

namespace Volt::RHI
{
	struct TrackerContainer
	{
		// Note: This should never be accessed, it's only for keeping a reference
		//		 to the resource.
		Variant<IntRef<RHIInterface>, IntRef<ArenaRHIInterface>> resource;
		class LastSubmissionTracker* submissionTracker;
		size_t hash;

		VT_INLINE friend bool operator==(const TrackerContainer& lhs, const TrackerContainer& rhs)
		{
			return lhs.hash == rhs.hash;
		}
	};
}

namespace std
{
	template<>
	struct hash<Volt::RHI::TrackerContainer>
	{
		std::size_t operator()(const Volt::RHI::TrackerContainer& container) const
		{
			return container.hash;
		}
	};
}

namespace Volt::RHI
{
	class LastSubmissionTrackerManager
	{
	public:
		template<typename T>
		void TryRegisterResource(IntRef<T> resource);

		void AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value);
		void Reset();

	private:
		std::unordered_set<TrackerContainer> m_registeredTrackers;
		std::unordered_set<TrackerContainer> m_registeredTrackersArena;
	};

	template<typename T>
	void LastSubmissionTrackerManager::TryRegisterResource(IntRef<T> resource)
	{
		if constexpr (std::is_base_of_v<LastSubmissionTracker, T>)
		{
			TrackerContainer container;

			if constexpr (std::is_base_of_v<RHIInterface, T>)
			{
				container.resource.Emplace<IntRef<RHIInterface>>(resource.template As<RHIInterface>());
			}
			else
			{
				container.resource.Emplace<IntRef<ArenaRHIInterface>>(resource.template As<ArenaRHIInterface>());
			}

			container.submissionTracker = static_cast<LastSubmissionTracker*>(resource.GetRaw());
			container.hash = resource.GetHash();

			m_registeredTrackers.insert(container);
		}
	}
}
