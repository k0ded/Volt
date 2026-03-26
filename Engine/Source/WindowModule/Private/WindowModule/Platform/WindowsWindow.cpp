#include "windowpch.h"

#include "WindowModule/Platform/WindowsWindow.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

namespace Volt
{
#if 0
	WindowsWindow::WindowsWindow(const WindowInitializer& initializer)
	{
		m_title = initializer.title;
		m_width = initializer.initialWidth;
		m_height = initializer.initialHeight;
		m_posX = initializer.initialPosX;
		m_posY = initializer.initialPosY;

		constexpr const char* ClassName = "VoltWindow";

		// Register window
		{
			WNDCLASSEXA windowClass{ 0 };
			windowClass.cbSize = sizeof(WNDCLASSEXA);
			windowClass.style = CS_CLASSDC;
			windowClass.lpfnWndProc = DefWindowProc;
			windowClass.cbClsExtra = 0;
			windowClass.cbWndExtra = 0;
			windowClass.hInstance = GetModuleHandle(NULL);
			windowClass.hIcon = nullptr;
			windowClass.hCursor = nullptr;
			windowClass.hbrBackground = nullptr;
			windowClass.lpszMenuName = nullptr;
			windowClass.lpszClassName = ClassName;
			windowClass.hIconSm = nullptr;

			RegisterClassExA(&windowClass);
		}

		// Create window
		{
			m_handle = CreateWindowExA(
				0,
				ClassName,
				m_title.c_str(),
				WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_SYSMENU | WS_VISIBLE,
				m_posX, m_posY,
				m_width, m_height,
				nullptr, nullptr,
				GetModuleHandle(NULL),
				nullptr
			);
		}


	}

	WindowsWindow::~WindowsWindow()
	{

	}
#endif
}
