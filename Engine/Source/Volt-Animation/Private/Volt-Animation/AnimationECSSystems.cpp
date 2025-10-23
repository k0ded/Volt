#include "Volt-Animation/AnimationComponents.h"
#include "Volt-Animation/Assets/Animation.h"

#include <AssetSystem/AssetManager.h>

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

		Ref<Animation> animation = AssetManager::QueueAsset<Animation>(animPlayerComp.animationHandle);
		if (!animation->IsValid())
		{
			return;
		}

		animPlayerComp.currentPlayTime += variableUpdate.deltaTime;
		if (animPlayerComp.currentPlayTime >= animation->GetDuration())
		{
			animPlayerComp.currentPlayTime -= animation->GetDuration();
		}
		
		const float fraction = animPlayerComp.currentPlayTime / animation->GetDuration();
		//const Vector<glm::mat4> pose = animation->Sample(fraction, /*Need a skelington here*/);
		//todo: put the pose on the animated mesh component here
	}

	void RegisterModule(ECSBuilder& builder)
	{
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(AnimationPlayerSystem);
	}

	VT_REGISTER_ECS_MODULE(RegisterModule);
}
