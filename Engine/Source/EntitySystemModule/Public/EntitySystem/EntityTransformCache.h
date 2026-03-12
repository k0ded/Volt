#pragma once

#include "EntitySystem/EntityID.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Math/TQS.h>

#include <shared_mutex>

namespace Volt
{
	class VTES_API EntityTransformCache
	{
	public:
		EntityTransformCache() = default;
		~EntityTransformCache() = default;

		void CacheTransform(EntityID entityId, const TQS& transform);
		void InvalidateTransform(EntityID entityId);

		bool TryGetCachedTransform(EntityID entityId, TQS& outTransform) const;

	private:
		using WriteLock = std::unique_lock<std::shared_mutex>;
		using ReadLock = std::shared_lock<std::shared_mutex>;

		Map<EntityID, TQS> m_transformCache;
		mutable std::shared_mutex m_mutex;
	};
}
