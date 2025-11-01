#include "vapch.h"

#include "Volt-Animation/AnimationComponents.h"
#include "Volt-Animation/Assets/Animation.h"
#include "Volt-Animation/TempAnimator.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <EntitySystem/Scripting/ECSBuilder.h>
#include <EntitySystem/Scripting/ECSSystemRegistry.h>
#include <EntitySystem/Scripting/CoreEnvironments.h>
namespace Volt
{
	using AnimationPlayerEntity = ECS::Access
		::Write<AnimationPlayerComponent>
		//::Write on the component that displays the pose
		::As<ECS::Type::Entity>;

	void AnimationPlayerSystem(AnimationPlayerEntity entity, const env::VariableUpdate& variableUpdate)
	{
		AnimationPlayerComponent& animPlayerComp = entity.GetComponent<AnimationPlayerComponent>();

		if (animPlayerComp.animationHandle == Asset::Null())
		{
			return;
		}

		AssetReference<Animation> animation;
		if (!g_assetManager->TryGetAsset<Animation>(animPlayerComp.animationHandle, animation))
		{
			return;
		}

		ScopedAssetReferenceLock animationLock{ animation };

		animPlayerComp.currentPlayTime += variableUpdate.deltaTime;
		if (animPlayerComp.currentPlayTime >= animation->GetDuration())
		{
			animPlayerComp.currentPlayTime -= animation->GetDuration();
		}
		
		animPlayerComp.animator->Update(animPlayerComp.currentPlayTime);
	}

	void RegisterModule(ECSBuilder& builder)
	{
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(AnimationPlayerSystem);
	}

	VT_REGISTER_ECS_MODULE(RegisterModule);
}
