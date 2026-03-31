#pragma once

#include "WindowModule/Config.h"
#include "WindowModule/WindowHandle.h"

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Pointers/Unique.h>

namespace Volt
{
	struct WindowInitializer;
	class Window_New;

	class WindowManager_New : public SubSystem
	{
	public:
		WindowManager_New();
		~WindowManager_New() override;

		WindowManager_New(const WindowManager_New&) = delete;
		WindowManager_New& operator=(const WindowManager_New&) = delete;

		WINDOWMODULE_API WindowHandle CreateWindow(const WindowInitializer& initializer);
		WINDOWMODULE_API void DestroyWindow(WindowHandle handle);

		WINDOWMODULE_API Window_New& GetWindow(WindowHandle handle);

		WINDOWMODULE_API void ProcessMessages();
		WINDOWMODULE_API void BeginFrame();
		WINDOWMODULE_API void Render(float timestep);
		WINDOWMODULE_API void Present();

		WINDOWMODULE_API void RepaintWindow(Window_New& window);

		WINDOWMODULE_API static WindowManager_New& Get();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{18B90ED5-09D0-435F-8EBD-69A12DB815CB}"_guid);

	private:
		inline static WindowManager_New* s_instance = nullptr;

		Map<WindowHandle, Unique<Window_New>> m_windows;
	};
}
