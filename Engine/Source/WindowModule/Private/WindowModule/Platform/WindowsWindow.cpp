#include "windowpch.h"

#include "WindowModule/Platform/WindowsWindow.h"
#include "WindowModule/Events/WindowEvents_New.h"

#include <EventSystem/EventSystem.h>

#include <CoreUtilities/Platform/Windows/VoltWindows.h>
#include <CoreUtilities/Profiling/Profiling.h>

#define LOG_WM 0
#if LOG_WM

#define WM_UAHDESTROYWINDOW 0x0090
#define WM_UAHDRAWMENU 0x0091
#define WM_UAHDRAWMENUITEM 0x0092
#define WM_UAHINITMENU 0x0093
#define WM_UAHMEASUREMENUITEM 0x0094
#define WM_UAHNCPAINTMENUPOPUP 0x0095

#define REGISTER_MESSAGE(msg){msg,#msg}

class WindowsMessageMap
{
public:
	WindowsMessageMap()
		: map({
			REGISTER_MESSAGE(WM_CREATE),
			REGISTER_MESSAGE(WM_DESTROY),
			REGISTER_MESSAGE(WM_MOVE),
			REGISTER_MESSAGE(WM_SIZE),
			REGISTER_MESSAGE(WM_ACTIVATE),
			REGISTER_MESSAGE(WM_SETFOCUS),
			REGISTER_MESSAGE(WM_KILLFOCUS),
			REGISTER_MESSAGE(WM_ENABLE),
			REGISTER_MESSAGE(WM_SETREDRAW),
			REGISTER_MESSAGE(WM_SETTEXT),
			REGISTER_MESSAGE(WM_GETTEXT),
			REGISTER_MESSAGE(WM_GETTEXTLENGTH),
			REGISTER_MESSAGE(WM_PAINT),
			REGISTER_MESSAGE(WM_CLOSE),
			REGISTER_MESSAGE(WM_QUERYENDSESSION),
			REGISTER_MESSAGE(WM_QUIT),
			REGISTER_MESSAGE(WM_QUERYOPEN),
			REGISTER_MESSAGE(WM_ERASEBKGND),
			REGISTER_MESSAGE(WM_SYSCOLORCHANGE),
			REGISTER_MESSAGE(WM_ENDSESSION),
			REGISTER_MESSAGE(WM_SHOWWINDOW),
			REGISTER_MESSAGE(WM_CTLCOLORMSGBOX),
			REGISTER_MESSAGE(WM_CTLCOLOREDIT),
			REGISTER_MESSAGE(WM_CTLCOLORLISTBOX),
			REGISTER_MESSAGE(WM_CTLCOLORBTN),
			REGISTER_MESSAGE(WM_CTLCOLORDLG),
			REGISTER_MESSAGE(WM_CTLCOLORSCROLLBAR),
			REGISTER_MESSAGE(WM_CTLCOLORSTATIC),
			REGISTER_MESSAGE(WM_WININICHANGE),
			REGISTER_MESSAGE(WM_SETTINGCHANGE),
			REGISTER_MESSAGE(WM_DEVMODECHANGE),
			REGISTER_MESSAGE(WM_ACTIVATEAPP),
			REGISTER_MESSAGE(WM_FONTCHANGE),
			REGISTER_MESSAGE(WM_TIMECHANGE),
			REGISTER_MESSAGE(WM_CANCELMODE),
			REGISTER_MESSAGE(WM_SETCURSOR),
			REGISTER_MESSAGE(WM_MOUSEACTIVATE),
			REGISTER_MESSAGE(WM_CHILDACTIVATE),
			REGISTER_MESSAGE(WM_QUEUESYNC),
			REGISTER_MESSAGE(WM_GETMINMAXINFO),
			REGISTER_MESSAGE(WM_ICONERASEBKGND),
			REGISTER_MESSAGE(WM_NEXTDLGCTL),
			REGISTER_MESSAGE(WM_SPOOLERSTATUS),
			REGISTER_MESSAGE(WM_DRAWITEM),
			REGISTER_MESSAGE(WM_MEASUREITEM),
			REGISTER_MESSAGE(WM_DELETEITEM),
			REGISTER_MESSAGE(WM_VKEYTOITEM),
			REGISTER_MESSAGE(WM_CHARTOITEM),
			REGISTER_MESSAGE(WM_SETFONT),
			REGISTER_MESSAGE(WM_GETFONT),
			REGISTER_MESSAGE(WM_QUERYDRAGICON),
			REGISTER_MESSAGE(WM_COMPAREITEM),
			REGISTER_MESSAGE(WM_COMPACTING),
			REGISTER_MESSAGE(WM_NCCREATE),
			REGISTER_MESSAGE(WM_NCDESTROY),
			REGISTER_MESSAGE(WM_NCCALCSIZE),
			REGISTER_MESSAGE(WM_NCHITTEST),
			REGISTER_MESSAGE(WM_NCPAINT),
			REGISTER_MESSAGE(WM_NCACTIVATE),
			REGISTER_MESSAGE(WM_GETDLGCODE),
			REGISTER_MESSAGE(WM_NCMOUSEMOVE),
			REGISTER_MESSAGE(WM_NCLBUTTONDOWN),
			REGISTER_MESSAGE(WM_NCLBUTTONUP),
			REGISTER_MESSAGE(WM_NCLBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_NCRBUTTONDOWN),
			REGISTER_MESSAGE(WM_NCRBUTTONUP),
			REGISTER_MESSAGE(WM_NCRBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_NCMBUTTONDOWN),
			REGISTER_MESSAGE(WM_NCMBUTTONUP),
			REGISTER_MESSAGE(WM_NCMBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_KEYDOWN),
			REGISTER_MESSAGE(WM_KEYUP),
			REGISTER_MESSAGE(WM_CHAR),
			REGISTER_MESSAGE(WM_DEADCHAR),
			REGISTER_MESSAGE(WM_SYSKEYDOWN),
			REGISTER_MESSAGE(WM_SYSKEYUP),
			REGISTER_MESSAGE(WM_SYSCHAR),
			REGISTER_MESSAGE(WM_SYSDEADCHAR),
			REGISTER_MESSAGE(WM_KEYLAST),
			REGISTER_MESSAGE(WM_INITDIALOG),
			REGISTER_MESSAGE(WM_COMMAND),
			REGISTER_MESSAGE(WM_SYSCOMMAND),
			REGISTER_MESSAGE(WM_TIMER),
			REGISTER_MESSAGE(WM_HSCROLL),
			REGISTER_MESSAGE(WM_VSCROLL),
			REGISTER_MESSAGE(WM_INITMENU),
			REGISTER_MESSAGE(WM_INITMENUPOPUP),
			REGISTER_MESSAGE(WM_MENUSELECT),
			REGISTER_MESSAGE(WM_MENUCHAR),
			REGISTER_MESSAGE(WM_ENTERIDLE),
			REGISTER_MESSAGE(WM_MOUSEWHEEL),
			REGISTER_MESSAGE(WM_MOUSEMOVE),
			REGISTER_MESSAGE(WM_LBUTTONDOWN),
			REGISTER_MESSAGE(WM_LBUTTONUP),
			REGISTER_MESSAGE(WM_LBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_RBUTTONDOWN),
			REGISTER_MESSAGE(WM_RBUTTONUP),
			REGISTER_MESSAGE(WM_RBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_MBUTTONDOWN),
			REGISTER_MESSAGE(WM_MBUTTONUP),
			REGISTER_MESSAGE(WM_MBUTTONDBLCLK),
			REGISTER_MESSAGE(WM_PARENTNOTIFY),
			REGISTER_MESSAGE(WM_MDICREATE),
			REGISTER_MESSAGE(WM_MDIDESTROY),
			REGISTER_MESSAGE(WM_MDIACTIVATE),
			REGISTER_MESSAGE(WM_MDIRESTORE),
			REGISTER_MESSAGE(WM_MDINEXT),
			REGISTER_MESSAGE(WM_MDIMAXIMIZE),
			REGISTER_MESSAGE(WM_MDITILE),
			REGISTER_MESSAGE(WM_MDICASCADE),
			REGISTER_MESSAGE(WM_MDIICONARRANGE),
			REGISTER_MESSAGE(WM_MDIGETACTIVE),
			REGISTER_MESSAGE(WM_MDISETMENU),
			REGISTER_MESSAGE(WM_CUT),
			REGISTER_MESSAGE(WM_COPYDATA),
			REGISTER_MESSAGE(WM_COPY),
			REGISTER_MESSAGE(WM_PASTE),
			REGISTER_MESSAGE(WM_CLEAR),
			REGISTER_MESSAGE(WM_UNDO),
			REGISTER_MESSAGE(WM_RENDERFORMAT),
			REGISTER_MESSAGE(WM_RENDERALLFORMATS),
			REGISTER_MESSAGE(WM_DESTROYCLIPBOARD),
			REGISTER_MESSAGE(WM_DRAWCLIPBOARD),
			REGISTER_MESSAGE(WM_PAINTCLIPBOARD),
			REGISTER_MESSAGE(WM_VSCROLLCLIPBOARD),
			REGISTER_MESSAGE(WM_SIZECLIPBOARD),
			REGISTER_MESSAGE(WM_ASKCBFORMATNAME),
			REGISTER_MESSAGE(WM_CHANGECBCHAIN),
			REGISTER_MESSAGE(WM_HSCROLLCLIPBOARD),
			REGISTER_MESSAGE(WM_QUERYNEWPALETTE),
			REGISTER_MESSAGE(WM_PALETTEISCHANGING),
			REGISTER_MESSAGE(WM_PALETTECHANGED),
			REGISTER_MESSAGE(WM_DROPFILES),
			REGISTER_MESSAGE(WM_POWER),
			REGISTER_MESSAGE(WM_WINDOWPOSCHANGED),
			REGISTER_MESSAGE(WM_WINDOWPOSCHANGING),
			REGISTER_MESSAGE(WM_HELP),
			REGISTER_MESSAGE(WM_NOTIFY),
			REGISTER_MESSAGE(WM_CONTEXTMENU),
			REGISTER_MESSAGE(WM_TCARD),
			REGISTER_MESSAGE(WM_MDIREFRESHMENU),
			REGISTER_MESSAGE(WM_MOVING),
			REGISTER_MESSAGE(WM_STYLECHANGED),
			REGISTER_MESSAGE(WM_STYLECHANGING),
			REGISTER_MESSAGE(WM_SIZING),
			REGISTER_MESSAGE(WM_SETHOTKEY),
			REGISTER_MESSAGE(WM_PRINT),
			REGISTER_MESSAGE(WM_PRINTCLIENT),
			REGISTER_MESSAGE(WM_POWERBROADCAST),
			REGISTER_MESSAGE(WM_HOTKEY),
			REGISTER_MESSAGE(WM_GETICON),
			REGISTER_MESSAGE(WM_EXITMENULOOP),
			REGISTER_MESSAGE(WM_ENTERMENULOOP),
			REGISTER_MESSAGE(WM_DISPLAYCHANGE),
			REGISTER_MESSAGE(WM_STYLECHANGED),
			REGISTER_MESSAGE(WM_STYLECHANGING),
			REGISTER_MESSAGE(WM_GETICON),
			REGISTER_MESSAGE(WM_SETICON),
			REGISTER_MESSAGE(WM_SIZING),
			REGISTER_MESSAGE(WM_MOVING),
			REGISTER_MESSAGE(WM_CAPTURECHANGED),
			REGISTER_MESSAGE(WM_DEVICECHANGE),
			REGISTER_MESSAGE(WM_PRINT),
			REGISTER_MESSAGE(WM_PRINTCLIENT),
			REGISTER_MESSAGE(WM_IME_SETCONTEXT),
			REGISTER_MESSAGE(WM_IME_NOTIFY),
			REGISTER_MESSAGE(WM_NCMOUSELEAVE),
			REGISTER_MESSAGE(WM_EXITSIZEMOVE),
			REGISTER_MESSAGE(WM_UAHDESTROYWINDOW),
			REGISTER_MESSAGE(WM_DWMNCRENDERINGCHANGED),
			REGISTER_MESSAGE(WM_ENTERSIZEMOVE),
		})
	{

	}

	String operator()(DWORD msg, LPARAM lp, WPARAM wp) const
	{
		const auto i = map.find(msg);

		if (i != map.end())
		{
			return i->second;
		}

		return "NULL";
	}
private:
	std::unordered_map<DWORD, String> map;
};
#endif

namespace Volt
{
	WindowsWindow::WindowsWindow(const WindowInitializer& initializer, WindowHandle handle)
		: m_inputManager(this),
		m_handle(handle)
	{
		m_title = initializer.title;
		m_width = initializer.initialWidth;
		m_height = initializer.initialHeight;
		m_posX = initializer.initialPosX;
		m_posY = initializer.initialPosY;
		m_enableVSync = initializer.enableVSync;

		constexpr DWORD WindowStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_SYSMENU;

		RECT windowRect;
		windowRect.left = 100;
		windowRect.right = m_width + windowRect.left;
		windowRect.top = 100;
		windowRect.bottom = m_height + windowRect.top;

		AdjustWindowRect(&windowRect, WindowStyle, FALSE);

		// Create window
		m_nativeHandle = CreateWindowExW(
			0,
			WindowClass::GetName(),
			m_title.c_str(),
			WindowStyle,
			m_posX, m_posY,
			windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
			nullptr, nullptr,
			GetModuleHandle(NULL),
			this
		);

		CreateSwapchain();

		ShowWindow(static_cast<HWND>(m_nativeHandle), SW_SHOWDEFAULT);
	}

	WindowsWindow::~WindowsWindow()
	{
		m_swapchain.Reset();

		DestroyWindow(static_cast<HWND>(m_nativeHandle));
	}
	
	void WindowsWindow::ProcessMessages()
	{
		VT_PROFILE_FUNCTION();
		m_inputManager.ClearFrameState();

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void WindowsWindow::BeginFrame()
	{
		m_swapchain->BeginFrame();
	}

	void WindowsWindow::Present()
	{
		m_swapchain->Present();
	}

	void WindowsWindow::SetTitle(const WString& title)
	{
		m_title = title;
		VT_CHECK(SetWindowTextW(static_cast<HWND>(m_nativeHandle), m_title.c_str()));
	}
	
	void WindowsWindow::SetXPosition(int32_t xPos)
	{
		m_posX = xPos;
		VT_MAYBE_UNUSED BOOL result = SetWindowPos(static_cast<HWND>(m_nativeHandle), nullptr, xPos, m_posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		VT_ASSERT(result == TRUE);
	}
	
	void WindowsWindow::SetYPosition(int32_t yPos)
	{
		m_posY = yPos;
		VT_MAYBE_UNUSED BOOL result = SetWindowPos(static_cast<HWND>(m_nativeHandle), nullptr, m_posX, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		VT_ASSERT(result == TRUE);
	}

	const WString& WindowsWindow::GetTitle() const
	{
		return m_title;
	}

	int32_t WindowsWindow::GetWidth() const
	{
		return m_width;
	}
	int32_t WindowsWindow::GetHeight() const
	{
		return m_height;
	}
	
	WindowHandle WindowsWindow::GetHandle() const
	{
		return m_handle;
	}

	void WindowsWindow::Maximize()
	{
		ShowWindow(static_cast<HWND>(m_nativeHandle), SW_MAXIMIZE);
	}
	
	void WindowsWindow::Minimize()
	{
		ShowWindow(static_cast<HWND>(m_nativeHandle), SW_MINIMIZE);
	}
	
	void WindowsWindow::Restore()
	{
		ShowWindow(static_cast<HWND>(m_nativeHandle), SW_RESTORE);
	}
	
	void WindowsWindow::Focus()
	{
		VT_MAYBE_UNUSED HWND result = SetFocus(static_cast<HWND>(m_nativeHandle));
		VT_ASSERT(result == m_nativeHandle);
	}
	
	bool WindowsWindow::IsMaximized() const
	{
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof(WINDOWPLACEMENT);

		VT_MAYBE_UNUSED BOOL result = GetWindowPlacement(static_cast<HWND>(m_nativeHandle), &windowPlacement);
		VT_ASSERT(result == TRUE);

		return windowPlacement.showCmd == SW_MAXIMIZE;
	}
	
	bool WindowsWindow::IsMinimized() const
	{
		WINDOWPLACEMENT windowPlacement;
		windowPlacement.length = sizeof(WINDOWPLACEMENT);

		VT_MAYBE_UNUSED BOOL result = GetWindowPlacement(static_cast<HWND>(m_nativeHandle), &windowPlacement);
		VT_ASSERT(result == TRUE);

		return windowPlacement.showCmd == SW_MINIMIZE;
	}
	
	bool WindowsWindow::IsFocused() const
	{
		HWND focusedHandle = GetFocus();
		return focusedHandle == m_nativeHandle;
	}

	WindowInputManager& WindowsWindow::GetInputManager()
	{
		return m_inputManager;
	}

	const RHI::Swapchain& WindowsWindow::GetSwapchain() const
	{
		return *m_swapchain;
	}

	LRESULT WindowsWindow::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_NCCREATE)
		{
			const CREATESTRUCTW* const createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			WindowsWindow* const window = static_cast<WindowsWindow*>(createStruct->lpCreateParams);

			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
			SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WindowsWindow::HandleMsgThunk));
		
			return window->HandleMsg(hWnd, msg, wParam, lParam);
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT WindowsWindow::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		WindowsWindow* const window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		return window->HandleMsg(hWnd, msg, wParam, lParam);
	}

	LRESULT WindowsWindow::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		VT_PROFILE_FUNCTION();

#if LOG_WM
		static WindowsMessageMap temp;
		VT_LOG(Trace, "{}", temp(msg, lParam, wParam));
#endif

		switch (msg)
		{
			case WM_CLOSE:
			{
				WindowCloseEvent_New closeEvent{ *this };
				EventSystem::DispatchEvent(closeEvent);
				return 0;
			}

			case WM_SIZE:
			{
				UINT x = LOWORD(lParam);
				UINT y = HIWORD(lParam);
				
				m_swapchain->Resize(x, y, m_enableVSync);
				
				WindowResizeEvent_New resizeEvent{ *this, x, y };
				EventSystem::DispatchEvent(resizeEvent);

				WindowRepaintEvent repaintEvent{ *this };
				EventSystem::DispatchEvent(repaintEvent);
				break;
			}
		}

		m_inputManager.HandleMessage(hWnd, msg, wParam, lParam);

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	void WindowsWindow::CreateSwapchain()
	{
		VT_PROFILE_FUNCTION();

		RHI::SwapchainCreateInfo createInfo{};
		createInfo.width = m_width;
		createInfo.height = m_height;
		createInfo.platformWindow = m_nativeHandle;
		createInfo.platformHandle = WindowClass::GetInstance();
		createInfo.useHDRIfAvailable = false; // #TODO_Ivar: Add HDR support.
		createInfo.enableVSync = m_enableVSync;

		m_swapchain = RHI::Swapchain::Create(createInfo);
	}

	WindowsWindow::WindowClass::WindowClass() noexcept
	{
		m_instance = GetModuleHandle(nullptr);
		
		WNDCLASSEXW windowClass{ 0 };
		windowClass.cbSize = sizeof(WNDCLASSEXW);
		windowClass.style = CS_CLASSDC;
		windowClass.lpfnWndProc = HandleMsgSetup;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = m_instance;
		windowClass.hIcon = nullptr;
		windowClass.hCursor = nullptr;
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = m_className;
		windowClass.hIconSm = nullptr;

		RegisterClassExW(&windowClass);
	}

	WindowsWindow::WindowClass::~WindowClass()
	{
		UnregisterClassW(GetName(), m_instance);
	}

	const wchar_t* WindowsWindow::WindowClass::GetName() noexcept
	{
		return Get().m_className;
	}
	
	HINSTANCE WindowsWindow::WindowClass::GetInstance() noexcept
	{
		return Get().m_instance;
	}
	
	WindowsWindow::WindowClass& WindowsWindow::WindowClass::Get()
	{
		static WindowClass instance;
		return instance;
	}
}
