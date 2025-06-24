#pragma once
//#include <map>
//#include <CoreUtilities/Containers/Vector.h>
#include "IWwiseAudioSystem.h"

namespace Volt
{
	namespace Audio
	{
		class WwiseAudioSystem : public IWwiseAudioSystem
		{
			using BankID = uint32_t;
			using BankMap = std::map<BankID, std::string>;

		public:
			WwiseAudioSystem();
			~WwiseAudioSystem() override = default;

			//ENGINE CONTROL
			bool Init() override;
			bool Release() override;
			void Update() override;
			void RuntimeStop() override;

			//BANK CONTROL
			BankID LoadBank(const char* bankName) override;
			bool UnloadBank(BankID bankID) override;

			//LISTENER CONTROL
			bool RegisterListener(uint32_t aEntityID, const char* aEntityName) override;
			bool UnregisterListener() override;
			bool SetListenerPosition(const AkTransform& transf) override;

			//OBJECT CONTROL
			bool RegisterObject(const char* eventName, AkGameObjectID objID) override;
			bool UnregisterObject(AkGameObjectID objID) override;
			bool SetObjectPosition(AkGameObjectID objID, const AkTransform& transf) override;

			//EVENT CONTROL
			bool PlayEvent(const char* eventName, AkGameObjectID objID, AkPlayingID& playID) override;
			bool SetRTPC(const char* eventName, float value, AkGameObjectID objID = 0, int32_t time = 0) override;
			bool ExecuteEventAction(uint32_t actionType, AkPlayingID playID) override;

			//GAME SYNCS
			virtual bool SetState(const char* aStateGroup, const char* aState) override;
			virtual bool SetSwitch(const char* aStateGroup, const char* aState, AkGameObjectID objID) override;

		private:


		private:
			BankMap m_loadedBanks;
			AkGameObjectID m_defaultListenerID = 0;

		};
	}
}
