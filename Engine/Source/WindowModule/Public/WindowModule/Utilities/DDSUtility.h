#pragma once

#include <CoreUtilities/Buffer/DataBuffer.h>

namespace std
{
	namespace filesystem
	{
		class path;
	}
}
namespace Volt
{
	class DDSUtility
	{
	public:
		struct TextureData
		{
			DataBuffer dataBuffer{};
			uint32_t width = 0;
			uint32_t height = 0;
		};

		static const TextureData GetRawDataFromDDS(const std::filesystem::path& path);

	private:
		DDSUtility() = delete;
	};
}
