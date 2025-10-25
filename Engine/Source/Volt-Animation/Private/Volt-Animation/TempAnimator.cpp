#include "vapch.h"

#include "Volt-Animation/TempAnimator.h"
#include <AssetSystem/AssetManager.h>

namespace Volt
{
	TempAnimator::TempAnimator(AssetHandle skeletonHandle, AssetHandle initialAnimationHandle)
	{
		if (skeletonHandle != Asset::Null())
		{
			m_skeleton = AssetManager::GetAsset<Skeleton>(skeletonHandle);
		}

		if (initialAnimationHandle != Asset::Null())
		{
			m_animation = AssetManager::GetAsset<Animation>(initialAnimationHandle);
		}
	}

	void TempAnimator::Update(float fraction)
	{
		m_fraction = fraction;
	}

	Vector<glm::mat4x4> TempAnimator::Sample()
	{
		return m_animation->Sample(m_fraction / m_animation->GetDuration(), m_skeleton, true);
	}
}
