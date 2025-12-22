#include "vaudiopch.h"
#include "Volt-Audio/Wwise/WwiseAudioSystem.h"
#include "Volt-Audio/AudioSystem/AudioSystemFactory.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/MusicEngine/Common/AkMusicEngine.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#include "ThirdParty/WWiseEngine/SoundEngine/Common/AkJobWorkerMgr.h"

#include "AK/SoundEngine/Common/AkMemoryMgr.h"		// Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Common/IAkStreamMgr.h>		// Streaming Manager
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>	// Music Engine
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>	// AkStreamMgrModule

#include <cassert>

namespace Volt
{
	namespace Audio
	{
		WwiseAudioSystem::WwiseAudioSystem()
		{
			m_audioBackend = AudioBackend::WWISE;
			Init();
			//TODO: Add Defaultpath to Audio Folder
			//TODO: Load Init.bank
		}

		bool WwiseAudioSystem::Init()
		{
			AkMemSettings memSettings;
			AK::MemoryMgr::GetDefaultSettings(memSettings);
			if (AK::MemoryMgr::Init(&memSettings) != AK_Success)
			{
				assert(!"Could not create the memory manager.");
				return false;
			}

			AkStreamMgrSettings streamMgrSettings;
			AK::StreamMgr::GetDefaultSettings(streamMgrSettings);
			if (!AK::StreamMgr::Create(streamMgrSettings))
			{
				assert(!"Could not create the Streaming Manager");
				return false;
			}
			AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)"));

			AkInitSettings initSettings;
			AK::SoundEngine::GetDefaultInitSettings(initSettings);
			AkPlatformInitSettings platformSettings;
			AK::SoundEngine::GetDefaultPlatformInitSettings(platformSettings);
			if (AK::SoundEngine::Init(&initSettings, &platformSettings) != AK_Success)
			{
				assert(!"Could not initialize the Sound Engine.");
				return false;
			}

			return true;
		}

		bool WwiseAudioSystem::Release()
		{
			// Unload all banks
			for (auto& bank : m_loadedBanks)
			{
				UnloadBank(bank.first);
			}

			AK::SoundEngine::UnregisterAllGameObj();
			AK::SoundEngine::Term();
			//myLowLevelIO->Term();

			if (AK::IAkStreamMgr::Get())
			{
				AK::IAkStreamMgr::Get()->Destroy();
			}

			AK::MemoryMgr::Term();

			return true;
		}

		void WwiseAudioSystem::Update()
		{
			AK::SoundEngine::RenderAudio();
		}

		void WwiseAudioSystem::RuntimeStop()
		{
			UnregisterListener();
		}

		WwiseAudioSystem::BankID WwiseAudioSystem::LoadBank(const char* bankFile)
		{
			AkBankID bankID;
			if (AK::SoundEngine::LoadBank(bankFile, bankID) != AK_Success)
			{
				return false;
			}
			m_loadedBanks.insert({ bankID, bankFile });
			return bankID;
		}

		bool WwiseAudioSystem::UnloadBank(BankID bankID)
		{
			if (auto it = m_loadedBanks.find(bankID); it != m_loadedBanks.end())
			{
				// return AK_Success if successful, AK_Fail otherwise. AK_Success is returned when the bank was not loaded.
				if (AK::SoundEngine::UnloadBank(it->second.c_str(), nullptr) != AK_Success)
				{
					return false;
				}
				m_loadedBanks.erase(bankID);
				return true;
			}
			return false; // Could not find bank
		}

		bool WwiseAudioSystem::RegisterListener(uint32_t aEntityID, const char* aEntityName)
		{
			if (AK::SoundEngine::RegisterGameObj(aEntityID, aEntityName) != AK_Success)
			{
				//ERROR MESSAGE
				return false;
			}

			m_defaultListenerID = aEntityID;
			if (AK::SoundEngine::SetDefaultListeners(&m_defaultListenerID, 1) != AK_Success)
			{
				//ERROR MESSAGE
				m_defaultListenerID = 0;
				return false;
			}

			return true;
		}

		bool WwiseAudioSystem::UnregisterListener()
		{
			if (AK::SoundEngine::RemoveDefaultListener(m_defaultListenerID) != AK_Success)
			{
				//ERROR MESSAGE
				return false;
			}

			return true;
		}

		bool WwiseAudioSystem::SetListenerPosition(const AkTransform& transf)
		{
			if (AK::SoundEngine::SetPosition(m_defaultListenerID, transf) != AK_Success)
			{
				//ERROR MESSAGE
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::RegisterObject(const char* eventName, AkGameObjectID objID)
		{
			if (AK::SoundEngine::RegisterGameObj(objID, eventName) != AK_Success)
			{
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::UnregisterObject(AkGameObjectID objID)
		{
			if (AK::SoundEngine::UnregisterGameObj(objID) != AK_Success)
			{
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::SetObjectPosition(AkGameObjectID objID, const AkTransform& transf)
		{
			if (AK::SoundEngine::SetPosition(objID, transf) != AK_Success)
			{
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::PlayEvent(const char* eventName, AkGameObjectID objID, AkPlayingID& out)
		{
			if (out = AK::SoundEngine::PostEvent(eventName, objID); out == AK_INVALID_PLAYING_ID)
			{
				assert(!"WWISE: Could not initialize a Event.");
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::SetRTPC(const char* RTPCName, float value, AkGameObjectID objID, int32_t time)
		{
			if (AK::SoundEngine::SetRTPCValue(RTPCName, value, objID, time) != AK_Success)
			{
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::ExecuteEventAction(uint32_t actionType, AkPlayingID playID)
		{
			const auto type = (AK::SoundEngine::AkActionOnEventType)actionType;
			AK::SoundEngine::ExecuteActionOnPlayingID(type, playID);
			return false;
		}

		bool WwiseAudioSystem::SetState(const char* aStateGroup, const char* aState)
		{
			if (AK::SoundEngine::SetState(aStateGroup, aState) != AK_Success)
			{
				return false;
			}
			return true;
		}

		bool WwiseAudioSystem::SetSwitch(const char* aStateGroup, const char* aState, AkGameObjectID objID)
		{
			if (AK::SoundEngine::SetSwitch(aStateGroup, aState, objID) != AK_Success)
			{
				return false;
			}
			return true;
		}
	}
}
