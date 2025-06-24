#pragma once

namespace Volt
{
	namespace Audio
	{
		class IAudioSystem;

		enum class AudioBackend
		{
			HEADLESS,
			WWISE,
			FMOD
		};

		class AudioSystemFactory
		{
		public:
			static std::unique_ptr<IAudioSystem> Create(AudioBackend backend);
		};
	}
}
