#include "vspch.h"

#include <Volt-Audio/AudioSystem/IAudioSystem.h>
#include <Volt-Audio/Components/AudioComponents.h>

#include <EntitySystem/Scripting/CommonComponent.h>
#include <EntitySystem/Scripting/ECSBuilder.h>
#include <EntitySystem/Scripting/ECSSystemRegistry.h>
#include <EntitySystem/Scripting/CoreEnvironments.h>

namespace Volt
{
	using AudioListenerEntity = ECS::Access
		::Read<Audio::AudioListenerComponent>
		::Read<TransformComponent>
		::As<ECS::Type::Entity>;

	void AudioListenerSystem(AudioListenerEntity entity, const env::VariableUpdate& variableUpdate)
	{
		//auto& listenerComponent = entity.GetComponent<Audio::AudioListenerComponent>();
		const auto& transform = entity.GetComponent<const TransformComponent>();
		if (transform.position.x != 0.f)
		{
			// Update AudioListner
		}
	}

	using AudioSourceEntity = ECS::Access
		::Read<Audio::AudioSourceComponent>
		::Read<TransformComponent>
		::As<ECS::Type::Entity>;

	void AudioSourceSystem(AudioSourceEntity entity, const env::VariableUpdate& variableUpdate)
	{
		//auto& listenerComponent = entity.GetComponent<Audio::AudioSourceComponent>();
		const auto& transform = entity.GetComponent<const TransformComponent>();
		if (transform.position.x != 0.f)
		{
			// Update AudioSource
		}
	}


	void RegisterAudioModule(ECSBuilder& builder)
	{
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(AudioListenerSystem);
		builder.GetGameLoop(GameLoop::Variable).RegisterSystem(AudioSourceSystem); 
	}

	VT_REGISTER_ECS_MODULE(RegisterAudioModule, "{354158B7-91D5-4039-8171-8513CDAA6141}"_guid);
}
