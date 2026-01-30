#include "espch.h"

#include "EntitySystem/EntityTransformCache.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	void EntityTransformCache::CacheTransform(EntityID entityId, const TQS& transform)
	{
		WriteLock lock{ m_mutex };
		m_transformCache[entityId] = transform;
	}

	void EntityTransformCache::InvalidateTransform(EntityID entityId)
	{
		WriteLock lock{ m_mutex };
		if (m_transformCache.contains(entityId))
		{
			m_transformCache.erase(entityId);
		}
	}

	bool EntityTransformCache::TryGetCachedTransform(EntityID entityId, TQS& outTransform) const
	{
		VT_PROFILE_FUNCTION();

		ReadLock lock{ m_mutex };

		auto it = m_transformCache.find(entityId);
		if (it != m_transformCache.end())
		{
			outTransform = it->second;
			return true;
		}

		return false;
	}
}
