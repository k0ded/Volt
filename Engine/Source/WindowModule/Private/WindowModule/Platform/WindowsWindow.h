#pragma once

#include "WindowModule/Window_New.h"

namespace Volt
{
#if 0
	class WindowsWindow : public Window_New
	{
	public:
		WindowsWindow(const WindowInitializer& initializer);
		~WindowsWindow() override;

	private:
		PlatformWindowHandle m_handle;

		String m_title;

		int32_t m_posX;
		int32_t m_posY;

		int32_t m_width;
		int32_t m_height;
	};
#endif
}
