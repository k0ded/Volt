#include "windowpch.h"

#include "WindowModule/Window_New.h"

#if VT_PLATFORM_WINDOWS
#include "WindowModule/Platform/WindowsWindow.h"
#endif

namespace Volt
{
	Unique<Window_New> Window_New::Create(const WindowInitializer& initializer)
	{
#if VT_PLATFORM_WINDOWS
		//return CreateUnique<WindowsWindow>(initializer);
		return nullptr;
#endif
	}
}
