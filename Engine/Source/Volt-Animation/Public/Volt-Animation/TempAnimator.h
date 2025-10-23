#pragma once

#include "Volt-Animation/Config.h"
#include "Volt-Animation/Assets/Skeleton.h"
#include "Volt-Animation/Assets/Animation.h"

namespace Volt
{
	class VTA_API TempAnimator
	{
	public:
		TempAnimator(AssetHandle skeletonHandle, AssetHandle initialAnimationHandle);
	
		void Update(float fraction);

		Vector<glm::mat4x4> Sample();
		VT_NODISCARD VT_INLINE Weak<Skeleton> GetSkeleton() const { return m_skeleton; }

	private:
		float m_fraction = 0.f;

		Ref<Skeleton> m_skeleton;
		Ref<Animation> m_animation;
	};
}
