#pragma once
#include "Volt-Animation/Config.h"

#include <AssetSystem/AssetTypes.h>
#include <AssetSystem/Asset_New.h>

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

namespace Volt
{
	class TempAnimator;

	struct AnimationPlayerComponent
	{
		using AnimationPlayerEntity = ECS::Access
			::Write<AnimationPlayerComponent>
			::As<ECS::Type::Entity>;

		AssetHandle skeletonHandle = Asset_New::Null();
		AssetHandle animationHandle = Asset_New::Null();
		float currentPlayTime = 0.f;

		Ref<TempAnimator> animator;

		static void ReflectType(TypeDesc<AnimationPlayerComponent>& reflect)
		{
			reflect.SetGUID("{45673840-5218-417D-A1C7-800A46711F23}"_guid);
			reflect.SetLabel("Animation Player Component");
			AssetHandle animationHandle = Asset_New::Null();
			reflect.AddMember(&AnimationPlayerComponent::skeletonHandle, "skeleton", "Skeleton", "", Asset_New::Null(), AssetTypes::Skeleton);
			reflect.AddMember(&AnimationPlayerComponent::animationHandle, "animationHandle", "Animation", "", Asset_New::Null(), AssetTypes::Animation);
			reflect.AddMember(&AnimationPlayerComponent::currentPlayTime, "currentPlayTime", "Current Play Time", "", 0.f);
			reflect.SetOnMemberChangedCallback(&AnimationPlayerComponent::OnMemberChanged);
		}

		REGISTER_COMPONENT(AnimationPlayerComponent);

		VTA_API static void OnMemberChanged(AnimationPlayerEntity entity);
	};
}
