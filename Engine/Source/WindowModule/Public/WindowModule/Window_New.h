#pragma once

#include "WindowModule/Config.h"
#include "WindowModule/WindowHandle.h"

#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/String/VoltString.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <CoreUtilities/Delegates/DelegateDeclarationHelpers.h>

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
		Filesystem::Path iconFilepath;

		uint32_t initialWidth = 1280;
		uint32_t initialHeight = 720;
		
		int32_t initialPosX = 0;
		int32_t initialPosY = 0;

		bool enableVSync : 1 = true;
		bool createAsDecorated : 1 = true;
	};

	class Window_New
	{
	public:
		DECLARE_DELEGATE_RetVal_TwoParams(bool, IsHoveringTitlebar, int32_t, int32_t);
		DECLARE_DELEGATE_RetVal_TwoParams(bool, IsHoveringMaximizeButton, int32_t, int32_t);

		DECLARE_MULTICAST_DELEGATE_OneParam(OnWindowClosed, Window_New&);
		DECLARE_MULTICAST_DELEGATE_OneParam(OnWindowRepaint, Window_New&);
		DECLARE_MULTICAST_DELEGATE_OneParam(OnWindowRender, Window_New&);
		DECLARE_MULTICAST_DELEGATE_ThreeParams(OnWindowResize, Window_New&, uint32_t, uint32_t);

		virtual ~Window_New() = default;

		virtual void ProcessMessages() = 0;
		virtual void BeginFrame() = 0;
		virtual void Present() = 0;

		virtual void Close() = 0;
		virtual void Render() = 0;

		virtual void SetTitle(const WString& title) = 0;

		virtual void SetPositionX(int32_t xPos) = 0;
		virtual void SetPositionY(int32_t yPos) = 0;

		virtual const WString& GetTitle() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual int32_t GetPositionX() const = 0;
		virtual int32_t GetPositionY() const = 0;
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

		virtual OnWindowClosed& GetOnWindowClosed() = 0;
		virtual OnWindowRepaint& GetOnWindowRepaint() = 0;
		virtual OnWindowRender& GetOnWindowRender() = 0;
		virtual OnWindowResize& GetOnWindowResize() = 0;

		// These are used if a window is not decorated.
		virtual IsHoveringTitlebar& GetIsHoveringTitlebar() = 0;
		virtual IsHoveringMaximizeButton& GetIsHoveringMaximizeButton() = 0;

		WINDOWMODULE_API static Unique<Window_New> Create(const WindowInitializer& initializer, WindowHandle handle);
	};
}
