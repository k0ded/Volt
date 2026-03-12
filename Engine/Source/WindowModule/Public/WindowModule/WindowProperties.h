#pragma once
#include "WindowModule/WindowMode.h"
#include <filesystem>

#include "Config.h"

namespace Volt
{
	struct WINDOWMODULE_API WindowProperties
	{
		std::string title = "Volt";
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
		std::filesystem::path iconPath;
		std::filesystem::path cursorPath;
	};
}
