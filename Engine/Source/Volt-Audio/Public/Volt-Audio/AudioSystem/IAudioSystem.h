#pragma once

namespace Volt
{
	namespace Audio
	{
		enum class AudioBackend;

		class IAudioSystem
		{
		public:
			virtual ~IAudioSystem() = default;

			//ENGINE CONTROL
			virtual bool Init() = 0;
			virtual bool Release() = 0;
			virtual void Update() = 0;
			virtual void RuntimeStop() = 0;

			//BANK CONTROL
			virtual uint32_t LoadBank(const char* bankName) = 0;
			virtual bool UnloadBank(uint32_t bankID ) = 0;

			//LISTENER CONTROL
			virtual bool RegisterListener(uint32_t id, const char* name) = 0;
			virtual bool UnregisterListener() = 0;

			[[nodiscard]] AudioBackend GetBackendType() { return m_audioBackend; }

		protected:
			AudioBackend m_audioBackend;
		};
	}
}
