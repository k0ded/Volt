#pragma once

#include "WindowModule/WindowMode.h"

#include <CoreUtilities/Filesystem/Path.h>

namespace Volt
{
	struct WindowProperties
	{
		String title = "Volt";
		uint32_t width = 1280;
		uint32_t height = 720;
		bool vsync = true;
		
		bool useTitlebar = false;
		bool useCustomTitlebar = false;
		bool createAsVisible = true;
		bool createAsFocused = true;
		bool focusOnShow = true;
		bool createAsDecorated = true;
		bool createAsAlwaysOnTop = false;

		WindowMode windowMode = WindowMode::Windowed;
		Filesystem::Path iconPath;
		Filesystem::Path cursorPath;
	};
}
