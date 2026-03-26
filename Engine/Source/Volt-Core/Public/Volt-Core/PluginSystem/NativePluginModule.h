#pragma once

#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	class NativePluginModule
	{
	public:
		NativePluginModule() = default;
		NativePluginModule(NativePluginModule&& other) noexcept;
		NativePluginModule(const Filesystem::Path& binaryFilepath, bool externallyLoaded) noexcept;

		NativePluginModule(const NativePluginModule& other) noexcept;
		NativePluginModule& operator=(const NativePluginModule& other) noexcept;

		~NativePluginModule();

		VT_NODISCARD VT_INLINE const Filesystem::Path& GetBinaryFilepath() const { return m_binaryFilepath; }

		void Unload();

	private:
		Filesystem::Path m_binaryFilepath;
		bool m_externallyLoaded = false;
	};
}
