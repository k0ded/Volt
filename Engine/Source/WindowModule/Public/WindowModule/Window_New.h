#pragma once

#include "WindowModule/Config.h"
#include "WindowModule/WindowHandle.h"

#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/String/VoltString.h>

#include <cstdint>

namespace Volt
{
	namespace RHI
	{
		class Swapchain;
	}

	class WindowInputManager;

	using PlatformWindowHandle = void*;

	struct WindowInitializer
	{
		WString title;

		int32_t initialWidth;
		int32_t initialHeight;
		
		int32_t initialPosX;
		int32_t initialPosY;

		bool enableVSync;
	};

	class Window_New
	{
	public:
		virtual ~Window_New() = default;

		virtual void ProcessMessages() = 0;
		virtual void BeginFrame() = 0;
		virtual void Present() = 0;

		virtual void SetTitle(const WString& title) = 0;

		virtual void SetXPosition(int32_t xPos) = 0;
		virtual void SetYPosition(int32_t yPos) = 0;

		virtual const WString& GetTitle() const = 0;

		virtual int32_t GetWidth() const = 0;
		virtual int32_t GetHeight() const = 0;
		virtual WindowHandle GetHandle() const = 0;

		virtual void Maximize() = 0;
		virtual void Minimize() = 0;
		virtual void Restore() = 0;
		virtual void Focus() = 0;

		virtual bool IsMaximized() const = 0;
		virtual bool IsMinimized() const = 0;
		virtual bool IsFocused() const = 0;

		virtual WindowInputManager& GetInputManager() = 0;
		virtual const RHI::Swapchain& GetSwapchain() const = 0;

		WINDOWMODULE_API static Unique<Window_New> Create(const WindowInitializer& initializer, WindowHandle handle);
	};
}
