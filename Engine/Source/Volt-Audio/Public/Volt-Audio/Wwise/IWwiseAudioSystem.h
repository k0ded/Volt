#pragma once
#include "Volt-Audio/AudioSystem/IAudioSystem.h"

namespace Volt
{
	namespace Audio
	{
		class IWwiseAudioSystem : public IAudioSystem
		{
			//LISTENER CONTROL
			virtual bool SetListenerPosition(const AkTransform& transf) = 0;

			//OBJECT CONTROL
			virtual bool RegisterObject(const char* eventName, AkGameObjectID objID) = 0;
			virtual bool UnregisterObject(AkGameObjectID objID) = 0;
			virtual bool SetObjectPosition(AkGameObjectID objID, const AkTransform& transf) = 0;

			//EVENT CONTROL
			virtual bool PlayEvent(const char* eventName, AkGameObjectID objID, AkPlayingID& playID) = 0;
			virtual bool SetRTPC(const char* eventName, float value, AkGameObjectID objID = 0, int32_t time = 0) = 0;
			virtual bool ExecuteEventAction(uint32_t actionType, AkPlayingID playID) = 0;

			//GAME SYNCS
			virtual bool SetState(const char* aStateGroup, const char* aState) = 0;
			virtual bool SetSwitch(const char* aStateGroup, const char* aState, AkGameObjectID objID) = 0;
		};
	}
}

