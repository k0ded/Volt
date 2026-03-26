#pragma once

#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/String/VoltString.h>

#include <cstdint>

namespace Volt
{
	using PlatformWindowHandle = void*;

	struct WindowInitializer
	{
		String title;

		int32_t initialWidth;
		int32_t initialHeight;
		
		int32_t initialPosX;
		int32_t initialPosY;
	};

	class Window_New
	{
	public:
		virtual ~Window_New() = default;
		virtual void SetTitle(const String& title) = 0;

		virtual void SetXPosition(int32_t xPos) = 0;
		virtual void SetYPosition(int32_t yPos) = 0;

		virtual const String& GetTitle() const = 0;

		virtual int32_t GetWidth() const = 0;
		virtual int32_t GetHeight() const = 0;

		virtual void Maximize() = 0;
		virtual void Minimize() = 0;
		virtual void Restore() = 0;
		virtual void Focus() = 0;

		virtual bool IsMaximized() const = 0;
		virtual bool IsMinimized() const = 0;
		virtual bool IsFocused() const = 0;

		static Unique<Window_New> Create(const WindowInitializer& initializer);
	};
}
