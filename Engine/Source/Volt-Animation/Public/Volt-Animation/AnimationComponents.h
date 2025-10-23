#pragma once
#include "Volt-Animation/Config.h"

#include <Volt-Core/AssetTypes.h>

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <AssetSystem/Asset.h>
#include <AssetSystem/AssetHandle.h>
namespace Volt
{
	class TempAnimator;

	struct AnimationPlayerComponent
	{
		using AnimationPlayerEntity = ECS::Access
			::Write<AnimationPlayerComponent>
			::As<ECS::Type::Entity>;

		AssetHandle skeletonHandle = Asset::Null();
		AssetHandle animationHandle = Asset::Null();
		float currentPlayTime = 0.f;

		Ref<TempAnimator> animator;

		static void ReflectType(TypeDesc<AnimationPlayerComponent>& reflect)
		{
			reflect.SetGUID("{45673840-5218-417D-A1C7-800A46711F23}"_guid);
			reflect.SetLabel("Animation Player Component");
			AssetHandle animationHandle = Asset::Null();
			reflect.AddMember(&AnimationPlayerComponent::skeletonHandle, "skeleton", "Skeleton", "", Asset::Null(), AssetTypes::Skeleton);
			reflect.AddMember(&AnimationPlayerComponent::animationHandle, "animationHandle", "Animation", "", Asset::Null(), AssetTypes::Animation);
			reflect.AddMember(&AnimationPlayerComponent::currentPlayTime, "currentPlayTime", "Current Play Time", "", 0.f);
			reflect.SetOnMemberChangedCallback(&AnimationPlayerComponent::OnMemberChanged);
		}

		REGISTER_COMPONENT(AnimationPlayerComponent);

		VTA_API static void OnMemberChanged(AnimationPlayerEntity entity);
	};
}
