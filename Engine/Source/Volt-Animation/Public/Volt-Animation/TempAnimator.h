#pragma once

#include "Volt-Animation/Config.h"
#include "Volt-Animation/Assets/Skeleton.h"
#include "Volt-Animation/Assets/Animation.h"

#include <AssetSystem/AssetReference.h>

namespace Volt
{
	class VTA_API TempAnimator
	{
	public:
		TempAnimator(AssetHandle skeletonHandle, AssetHandle initialAnimationHandle);
	
		void Update(float fraction);

		Vector<glm::mat4x4> Sample();
		VT_NODISCARD VT_INLINE AssetReference<Skeleton> GetSkeleton() const { return m_skeleton; }

	private:
		float m_fraction = 0.f;

		AssetReference<Skeleton> m_skeleton;
		AssetReference<Animation> m_animation;
	};
}
