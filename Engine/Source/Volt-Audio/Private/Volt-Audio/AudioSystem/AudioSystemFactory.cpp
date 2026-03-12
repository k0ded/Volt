#include "vaudiopch.h"
#include "Volt-Audio\AudioSystem\IAudioSystem.h"
#include "Volt-Audio\AudioSystem\AudioSystemFactory.h"
#include "Wwise\WwiseAudioSystem.h"

namespace Volt
{
	namespace Audio
	{
		std::unique_ptr<IAudioSystem> AudioSystemFactory::Create(AudioBackend backend)
		{
			switch (backend)
			{
				case AudioBackend::WWISE:
					return std::make_unique<WwiseAudioSystem>();
				default:
					return nullptr;
			}
		}
	}
}
