#include "vapch.h"
#include "Volt-Animation/AnimationComponents.h"

namespace Volt
{
	VT_REGISTER_COMPONENT(AnimationPlayerComponent);

	void AnimationPlayerComponent::OnMemberChanged(AnimationPlayerEntity entity)
	{
		//todo: we would want to set the pose of the animation here when that is possible to easily preview it in editor
	}
}
