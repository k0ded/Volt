#pragma once

#include "Volt-Audio/AudioSystem/IAudioSystem.h"

#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	namespace Audio
	{
		struct AudioListenerComponent
		{
		private:
			using AudioListenerEntity = ECS::Access
				::Read<Audio::AudioListenerComponent>
				::Read<TransformComponent>
				::As<ECS::Type::Entity>;

		public:
			static void ReflectType(TypeDesc<AudioListenerComponent>& reflect)
			{
				reflect.SetGUID("{D47FC3B6-1316-4008-B69F-15EBD2A6796A}"_guid);
				reflect.SetLabel("Audio Listener Component");
			}

			static void OnCreate(AudioListenerEntity entity);
			static void OnDestroy(AudioListenerEntity entity);


			REGISTER_COMPONENT(AudioListenerComponent);
		};

		struct AudioSourceComponent
		{
		private:
			using AudioSourceEntity = ECS::Access
				::Read<Audio::AudioSourceComponent>
				::Read<TransformComponent>
				::As<ECS::Type::Entity>;

		public:
			static void ReflectType(TypeDesc<AudioSourceComponent>& reflect)
			{
				reflect.SetGUID("{047C646B-62A8-4A46-86E6-A9A0B7306B56}"_guid);
				reflect.SetLabel("Audio Source Component");
			}

			static void OnCreate(AudioSourceEntity entity);
			static void OnDestroy(AudioSourceEntity entity);

			REGISTER_COMPONENT(AudioSourceComponent);
		};
	}
}
