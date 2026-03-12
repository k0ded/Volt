#include "vapch.h"

#include "Volt-Animation/TempAnimator.h"

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	TempAnimator::TempAnimator(AssetHandle skeletonHandle, AssetHandle initialAnimationHandle)
	{
		if (skeletonHandle != Asset::Null())
		{
			m_skeleton = g_assetManager->GetAssetImmediately<Skeleton>(skeletonHandle);
		}

		if (initialAnimationHandle != Asset::Null())
		{
			m_animation = g_assetManager->GetAssetImmediately<Animation>(initialAnimationHandle);
		}
	}

	void TempAnimator::Update(float fraction)
	{
		m_fraction = fraction;
	}

	Vector<glm::mat4x4> TempAnimator::Sample()
	{
		if (!m_animation)
		{
			return Vector<glm::mat4x4>();
		}
		return m_animation->Sample(m_fraction / m_animation->GetDuration(), *m_skeleton, true);
	}
}
