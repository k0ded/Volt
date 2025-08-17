#include "Volt-ImGui/ImGuiPlatform.h"

#include <WindowModule/Events/WindowEvents.h>
#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>
#include <WindowModule/Monitor.h>

#include <EventSystem/ApplicationEvents.h>

#include <InputModule/Events/KeyboardEvents.h>
#include <InputModule/Events/MouseEvents.h>

#include <LogModule/Log.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace Volt
{
	static int32_t TranslateUntranslatedKey(int32_t key, int32_t scancode)
	{
		if (key >= InputCodeToGLFWCode(InputCode::Numpad_0) && key <= InputCodeToGLFWCode(InputCode::Numpad_Equal))
			return key;
		
		const char* key_name = GetKeyName(GLFWKeyCodeToInputCode(key), scancode);

		if (key_name && key_name[0] != 0 && key_name[1] == 0)
		{
			const char char_names[] = "`-=[]\\,;\'./";
			const int char_keys[] = { InputCodeToGLFWCode(InputCode::GraveAccent), InputCodeToGLFWCode(InputCode::Minus), InputCodeToGLFWCode(InputCode::Equal), InputCodeToGLFWCode(InputCode::LeftBracket), InputCodeToGLFWCode(InputCode::RightBracket), InputCodeToGLFWCode(InputCode::Backslash), InputCodeToGLFWCode(InputCode::Comma), InputCodeToGLFWCode(InputCode::Semicolon), InputCodeToGLFWCode(InputCode::Apostrophe), InputCodeToGLFWCode(InputCode::Period), InputCodeToGLFWCode(InputCode::Slash), 0 };
			IM_ASSERT(IM_ARRAYSIZE(char_names) == IM_ARRAYSIZE(char_keys));
			if (key_name[0] >= '0' && key_name[0] <= '9') { key = InputCodeToGLFWCode(InputCode::Key_0) + (key_name[0] - '0'); }
			else if (key_name[0] >= 'A' && key_name[0] <= 'Z') { key = InputCodeToGLFWCode(InputCode::A) + (key_name[0] - 'A'); }
			else if (key_name[0] >= 'a' && key_name[0] <= 'z') { key = InputCodeToGLFWCode(InputCode::A) + (key_name[0] - 'a'); }
			else if (const char* p = strchr(char_names, key_name[0])) { key = char_keys[p - char_names]; }
		}
		// if (action == GLFW_PRESS) printf("key %d scancode %d name '%s'\n", key, scancode, key_name);
		return key;
	}

	static ImGuiKey InputCodeToImGuiKey(InputCode inputCode)
	{
		switch (inputCode)
		{
			case InputCode::Tab: return ImGuiKey_Tab;
			case InputCode::LeftArrow: return ImGuiKey_LeftArrow;
			case InputCode::RightArrow: return ImGuiKey_RightArrow;
			case InputCode::UpArrow: return ImGuiKey_UpArrow;
			case InputCode::DownArrow: return ImGuiKey_DownArrow;
			case InputCode::PageUp: return ImGuiKey_PageUp;
			case InputCode::PageDown: return ImGuiKey_PageDown;
			case InputCode::Home: return ImGuiKey_Home;
			case InputCode::End: return ImGuiKey_End;
			case InputCode::Insert: return ImGuiKey_Insert;
			case InputCode::Delete: return ImGuiKey_Delete;
			case InputCode::Backspace: return ImGuiKey_Backspace;
			case InputCode::Spacebar: return ImGuiKey_Space;
			case InputCode::Return: return ImGuiKey_Enter;
			case InputCode::Esc: return ImGuiKey_Escape;
			case InputCode::Apostrophe: return ImGuiKey_Apostrophe;
			case InputCode::Comma: return ImGuiKey_Comma;
			case InputCode::Minus: return ImGuiKey_Minus;
			case InputCode::Period: return ImGuiKey_Period;
			case InputCode::Slash: return ImGuiKey_Slash;
			case InputCode::Semicolon: return ImGuiKey_Semicolon;
			case InputCode::Equal: return ImGuiKey_Equal;
			case InputCode::LeftBracket: return ImGuiKey_LeftBracket;
			case InputCode::Backslash: return ImGuiKey_Backslash;
			case InputCode::World_1: return ImGuiKey_Oem102;
			case InputCode::World_2: return ImGuiKey_Oem102;
			case InputCode::RightBracket: return ImGuiKey_RightBracket;
			case InputCode::GraveAccent: return ImGuiKey_GraveAccent;
			case InputCode::CapsLock: return ImGuiKey_CapsLock;
			case InputCode::ScrollLock: return ImGuiKey_ScrollLock;
			case InputCode::NumLock: return ImGuiKey_NumLock;
			case InputCode::PrintScreen: return ImGuiKey_PrintScreen;
			case InputCode::Pause: return ImGuiKey_Pause;
			case InputCode::Numpad_0: return ImGuiKey_Keypad0;
			case InputCode::Numpad_1: return ImGuiKey_Keypad1;
			case InputCode::Numpad_2: return ImGuiKey_Keypad2;
			case InputCode::Numpad_3: return ImGuiKey_Keypad3;
			case InputCode::Numpad_4: return ImGuiKey_Keypad4;
			case InputCode::Numpad_5: return ImGuiKey_Keypad5;
			case InputCode::Numpad_6: return ImGuiKey_Keypad6;
			case InputCode::Numpad_7: return ImGuiKey_Keypad7;
			case InputCode::Numpad_8: return ImGuiKey_Keypad8;
			case InputCode::Numpad_9: return ImGuiKey_Keypad9;
			case InputCode::Decimal: return ImGuiKey_KeypadDecimal;
			case InputCode::Divide: return ImGuiKey_KeypadDivide;
			case InputCode::Multiply: return ImGuiKey_KeypadMultiply;
			case InputCode::Subtract: return ImGuiKey_KeypadSubtract;
			case InputCode::Add: return ImGuiKey_KeypadAdd;
			case InputCode::Enter: return ImGuiKey_KeypadEnter;
			case InputCode::Numpad_Equal: return ImGuiKey_KeypadEqual;
			case InputCode::LeftShift: return ImGuiKey_LeftShift;
			case InputCode::LeftControl: return ImGuiKey_LeftCtrl;
			case InputCode::LeftAlt: return ImGuiKey_LeftAlt;
			case InputCode::LeftSuper: return ImGuiKey_LeftSuper;
			case InputCode::RightShift: return ImGuiKey_RightShift;
			case InputCode::RightControl: return ImGuiKey_RightCtrl;
			case InputCode::RightAlt: return ImGuiKey_RightAlt;
			case InputCode::RightSuper: return ImGuiKey_RightSuper;
			case InputCode::Menu: return ImGuiKey_Menu;
			case InputCode::Key_0: return ImGuiKey_0;
			case InputCode::Key_1: return ImGuiKey_1;
			case InputCode::Key_2: return ImGuiKey_2;
			case InputCode::Key_3: return ImGuiKey_3;
			case InputCode::Key_4: return ImGuiKey_4;
			case InputCode::Key_5: return ImGuiKey_5;
			case InputCode::Key_6: return ImGuiKey_6;
			case InputCode::Key_7: return ImGuiKey_7;
			case InputCode::Key_8: return ImGuiKey_8;
			case InputCode::Key_9: return ImGuiKey_9;
			case InputCode::A: return ImGuiKey_A;
			case InputCode::B: return ImGuiKey_B;
			case InputCode::C: return ImGuiKey_C;
			case InputCode::D: return ImGuiKey_D;
			case InputCode::E: return ImGuiKey_E;
			case InputCode::F: return ImGuiKey_F;
			case InputCode::G: return ImGuiKey_G;
			case InputCode::H: return ImGuiKey_H;
			case InputCode::I: return ImGuiKey_I;
			case InputCode::J: return ImGuiKey_J;
			case InputCode::K: return ImGuiKey_K;
			case InputCode::L: return ImGuiKey_L;
			case InputCode::M: return ImGuiKey_M;
			case InputCode::N: return ImGuiKey_N;
			case InputCode::O: return ImGuiKey_O;
			case InputCode::P: return ImGuiKey_P;
			case InputCode::Q: return ImGuiKey_Q;
			case InputCode::R: return ImGuiKey_R;
			case InputCode::S: return ImGuiKey_S;
			case InputCode::T: return ImGuiKey_T;
			case InputCode::U: return ImGuiKey_U;
			case InputCode::V: return ImGuiKey_V;
			case InputCode::W: return ImGuiKey_W;
			case InputCode::X: return ImGuiKey_X;
			case InputCode::Y: return ImGuiKey_Y;
			case InputCode::Z: return ImGuiKey_Z;
			case InputCode::F1: return ImGuiKey_F1;
			case InputCode::F2: return ImGuiKey_F2;
			case InputCode::F3: return ImGuiKey_F3;
			case InputCode::F4: return ImGuiKey_F4;
			case InputCode::F5: return ImGuiKey_F5;
			case InputCode::F6: return ImGuiKey_F6;
			case InputCode::F7: return ImGuiKey_F7;
			case InputCode::F8: return ImGuiKey_F8;
			case InputCode::F9: return ImGuiKey_F9;
			case InputCode::F10: return ImGuiKey_F10;
			case InputCode::F11: return ImGuiKey_F11;
			case InputCode::F12: return ImGuiKey_F12;
			case InputCode::F13: return ImGuiKey_F13;
			case InputCode::F14: return ImGuiKey_F14;
			case InputCode::F15: return ImGuiKey_F15;
			case InputCode::F16: return ImGuiKey_F16;
			case InputCode::F17: return ImGuiKey_F17;
			case InputCode::F18: return ImGuiKey_F18;
			case InputCode::F19: return ImGuiKey_F19;
			case InputCode::F20: return ImGuiKey_F20;
			case InputCode::F21: return ImGuiKey_F21;
			case InputCode::F22: return ImGuiKey_F22;
			case InputCode::F23: return ImGuiKey_F23;
			case InputCode::F24: return ImGuiKey_F24;
			default: return ImGuiKey_None;
		}
	}

	ImGuiPlatform::ImGuiPlatform()
	{
		Initialize();
		RegisterEventListeners();
	}

	ImGuiPlatform::~ImGuiPlatform()
	{

	}

	void ImGuiPlatform::Destroy()
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendPlatformUserData = nullptr;
	}

	void ImGuiPlatform::BeginFrame()
	{
		ImGuiIO& io = ImGui::GetIO();
		Window& mainWindow = WindowManager::Get().GetMainWindow();

		{
			const uint32_t width = mainWindow.GetWidth();
			const uint32_t height = mainWindow.GetHeight();

			io.DisplaySize = { static_cast<float>(width), static_cast<float>(height) };

			if (width > 0 && height > 0)
			{
				auto [fbW, fbH] = mainWindow.GetFramebufferSize();
				io.DisplayFramebufferScale = { static_cast<float>(width) / static_cast<float>(fbW), static_cast<float>(height) / static_cast<float>(fbH) };
			}
			else
			{
				io.DisplayFramebufferScale = { 1.f, 1.f };
			}
		}

		io.DeltaTime = m_deltaTime;

		UpdateMouseData();
		UpdateMouseCursor();
	}

	void ImGuiPlatform::AddViewportWindow(Window* window)
	{
		ContextData contextData;
		contextData.imguiContext = ImGui::GetCurrentContext();
		contextData.lastValidMousePos = 0.f;

		m_windowToContextMap[window] = contextData;
	}

	void ImGuiPlatform::RemoveViewportWindow(Window* window)
	{
		m_windowToContextMap.erase(window);
	}

	static void UpdateImGuiModifierKeys(ImGuiIO& io, InputModifier modifiers)
	{
		io.AddKeyEvent(ImGuiMod_Ctrl, EnumValueContainsFlag(modifiers, InputModifier::Control));
		io.AddKeyEvent(ImGuiMod_Shift, EnumValueContainsFlag(modifiers, InputModifier::Shift));
		io.AddKeyEvent(ImGuiMod_Alt, EnumValueContainsFlag(modifiers, InputModifier::Alt));
		io.AddKeyEvent(ImGuiMod_Super, EnumValueContainsFlag(modifiers, InputModifier::Super));
	}

	void ImGuiPlatform::RegisterEventListeners()
	{
		RegisterListener<WindowFocusChangedEvent>([this](WindowFocusChangedEvent& event)
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
			
			ImGuiIO& io = ImGui::GetIO(context);
			io.AddFocusEvent(event.Focused());

			return false;
		});

		RegisterListener<WindowCursorEnteredEvent>([this](WindowCursorEnteredEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			ContextData& contextData = GetContextDataFromWindow(event.GetWindow());

			ImGuiIO& io = ImGui::GetIO(contextData.imguiContext);
			if (event.Entered())
			{
				contextData.mouseWindow = &event.GetWindow();
				io.AddMousePosEvent(contextData.lastValidMousePos.x, contextData.lastValidMousePos.y);
			}
			else if (!event.Entered() && contextData.mouseWindow == &event.GetWindow())
			{
				contextData.lastValidMousePos.x = io.MousePos.x;
				contextData.lastValidMousePos.y = io.MousePos.y;
				contextData.mouseWindow = nullptr;
				io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
			}

			return false;
		});

		RegisterListener<MouseMovedEvent>([this](MouseMovedEvent& event)
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			ContextData& contextData = GetContextDataFromWindow(event.GetWindow());

			float x = event.GetX();
			float y = event.GetY();

			ImGuiIO& io = ImGui::GetIO(contextData.imguiContext);
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				auto [windowX, windowY] = event.GetWindow().GetPosition();
				x += windowX;
				y += windowY;
			}

			io.AddMousePosEvent(x, y);
			contextData.lastValidMousePos = { x, y };

			return false;
		});

		RegisterListener<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			int32_t glfwCode = InputCodeToGLFWCode(event.GetMouseButton());

			if (event.GetMouseButton() != InputCode::Unknown && glfwCode < ImGuiMouseButton_COUNT)
			{
				ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
				ImGuiIO& io = ImGui::GetIO(context);

				io.AddMouseButtonEvent(glfwCode, true);
				UpdateImGuiModifierKeys(io, event.GetModifiers());
			}

			return false;
		});

		RegisterListener<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			int32_t glfwCode = InputCodeToGLFWCode(event.GetMouseButton());

			if (event.GetMouseButton() != InputCode::Unknown && glfwCode < ImGuiMouseButton_COUNT)
			{
				ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
				ImGuiIO& io = ImGui::GetIO(context);

				io.AddMouseButtonEvent(glfwCode, false);
				UpdateImGuiModifierKeys(io, event.GetModifiers());
			}

			return false;
		});

		RegisterListener<MouseScrolledEvent>([this](MouseScrolledEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
			ImGuiIO& io = ImGui::GetIO(context);

			io.AddMouseWheelEvent(event.GetXOffset(), event.GetYOffset());

			return false;
		});

		RegisterListener<KeyPressedEvent>([this](KeyPressedEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			if (event.GetRepeatCount() > 0)
			{
				return false;
			}

			int32_t keycode = TranslateUntranslatedKey(InputCodeToGLFWCode(event.GetKeyCode()), event.GetScanCode());
			ImGuiKey imguiKey = InputCodeToImGuiKey(GLFWKeyCodeToInputCode(keycode));

			ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
			ImGuiIO& io = ImGui::GetIO(context);
			
			io.AddKeyEvent(imguiKey, true);
			io.SetKeyEventNativeData(imguiKey, keycode, event.GetScanCode());

			UpdateImGuiModifierKeys(io, event.GetModifiers());

			return false;
		});

		RegisterListener<KeyReleasedEvent>([this](KeyReleasedEvent& event)
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			int32_t keycode = TranslateUntranslatedKey(InputCodeToGLFWCode(event.GetKeyCode()), event.GetScanCode());
			ImGuiKey imguiKey = InputCodeToImGuiKey(GLFWKeyCodeToInputCode(keycode));

			ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
			ImGuiIO& io = ImGui::GetIO(context);

			io.AddKeyEvent(imguiKey, false);
			io.SetKeyEventNativeData(imguiKey, keycode, event.GetScanCode());

			UpdateImGuiModifierKeys(io, event.GetModifiers());

			return false;
		});

		RegisterListener<KeyTypedEvent>([this](KeyTypedEvent& event) 
		{
			if (!IsWindowInContext(event.GetWindow()))
			{
				return false;
			}

			ImGuiContext* context = GetImGuiContextFromWindow(event.GetWindow());
			ImGuiIO& io = ImGui::GetIO(context);
			io.AddInputCharacter(event.GetCharacter());

			return false;
		});

		RegisterListener<AppTickEvent>([this](AppTickEvent& event)
		{
			m_deltaTime = event.GetTimestep();
			return false;
		});
	}

	void ImGuiPlatform::Initialize()
	{
		ImGuiIO& io = ImGui::GetIO();
		IMGUI_CHECKVERSION();

		io.BackendPlatformUserData = (void*)this;
		io.BackendPlatformName = "Volt";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
		io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;

		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) 
		{
			WindowManager::Get().GetMainWindow().SetClipboard(text);
		};

		platformIO.Platform_GetClipboardTextFn = [](ImGuiContext*) -> const char*
		{
			const std::string_view clipboard = WindowManager::Get().GetMainWindow().GetClipboard();
			return clipboard.data();
		};

		Window& mainWindow = WindowManager::Get().GetMainWindow();

		// Setup main viewport
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		mainViewport->PlatformHandle = &mainWindow;
		mainViewport->PlatformHandleRaw = mainWindow.GetNativeWindow();

		ContextData contextData;
		contextData.imguiContext = ImGui::GetCurrentContext();
		contextData.lastValidMousePos = 0.f;

		m_windowToContextMap[&mainWindow] = contextData;

		InitializeMonitors();
		InitializeViewportSupport();
	}

	static void CreateImGuiWindow(ImGuiViewport* viewport)
	{
		WindowProperties windowProperties;
		windowProperties.createAsVisible = false;
		windowProperties.createAsFocused = false;
		windowProperties.focusOnShow = false;
		windowProperties.createAsDecorated = (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true;
		windowProperties.createAsAlwaysOnTop = (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false;
		windowProperties.width = static_cast<uint32_t>(viewport->Size.x);
		windowProperties.height = static_cast<uint32_t>(viewport->Size.y);
		windowProperties.title = "No title";

		WindowHandle windowHandle = WindowManager::Get().CreateNewWindow(windowProperties);
		Window& window = WindowManager::Get().GetWindow(windowHandle);
		window.SetPosition(static_cast<int32_t>(viewport->Pos.x), static_cast<int32_t>(viewport->Pos.y));

		viewport->PlatformHandle = &window;

		ImGuiIO& io = ImGui::GetIO();
		ImGuiPlatform* imguiPlatform = (ImGuiPlatform*)io.BackendPlatformUserData;
		imguiPlatform->AddViewportWindow(&window);
	}

	static void DestroyImGuiWindow(ImGuiViewport* viewport)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiPlatform* imguiPlatform = (ImGuiPlatform*)io.BackendPlatformUserData;

		Window* window = (Window*)viewport->PlatformHandle;

		if (imguiPlatform)
		{
			imguiPlatform->RemoveViewportWindow(window);
		}

		// Only destroy windows that were created by imgui.
		Window* mainWindow = &WindowManager::Get().GetMainWindow();
		if (window != mainWindow)
		{
			WindowManager::Get().DestroyWindow(*window);
		}
	}

	static void ShowImGuiWindow(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->Show();
	}

	static void SetImGuiWindowPosition(ImGuiViewport* viewport, ImVec2 pos)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->SetPosition(static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y));
	}

	static ImVec2 GetImGuiWindowPosition(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		auto [x, y] = window->GetPosition();

		return { x, y };
	}

	static void SetImGuiWindowSize(ImGuiViewport* viewport, ImVec2 size)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->Resize(static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
	}

	static ImVec2 GetImGuiWindowSize(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		return { static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()) };
	}

	static ImVec2 GetImGuiWindowFramebufferScale(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
	
		const uint32_t width = window->GetWidth();
		const uint32_t height = window->GetHeight();

		ImVec2 framebufferScale = { 1.f, 1.f };

		if (width > 0 && height > 0)
		{
			auto [fbW, fbH] = window->GetFramebufferSize();
			framebufferScale = { static_cast<float>(width) / static_cast<float>(fbW), static_cast<float>(height) / static_cast<float>(fbH) };
		}

		return framebufferScale;
	}

	static void SetImGuiWindowFocus(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->Focus();
	}

	static bool GetImGuiWindowFocus(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		return window->IsFocused();
	}

	static bool GetImGuiWindowMinimized(ImGuiViewport* viewport)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		return window->IsMinimized();
	}

	static void SetImGuiWindowTitle(ImGuiViewport* viewport, const char* title)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->SetTitle(title);
	}

	static void SetImGuiWindowAlpha(ImGuiViewport* viewport, float alpha)
	{
		Window* window = (Window*)viewport->PlatformHandle;
		window->SetOpacity(alpha);
	}

	static void Unused_RenderImGuiWindow(ImGuiViewport*, void*)
	{}

	static void Unused_ImGuiWindowSwapBuffers(ImGuiViewport* viewport, void*)
	{}

	void ImGuiPlatform::InitializeViewportSupport()
	{
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Platform_CreateWindow = CreateImGuiWindow;
		platformIO.Platform_DestroyWindow = DestroyImGuiWindow;
		platformIO.Platform_ShowWindow = ShowImGuiWindow;
		platformIO.Platform_SetWindowPos = SetImGuiWindowPosition;
		platformIO.Platform_GetWindowPos = GetImGuiWindowPosition;
		platformIO.Platform_SetWindowSize = SetImGuiWindowSize;
		platformIO.Platform_GetWindowSize = GetImGuiWindowSize;
		platformIO.Platform_GetWindowFramebufferScale = GetImGuiWindowFramebufferScale;
		platformIO.Platform_SetWindowFocus = SetImGuiWindowFocus;
		platformIO.Platform_GetWindowFocus = GetImGuiWindowFocus;
		platformIO.Platform_GetWindowMinimized = GetImGuiWindowMinimized;
		platformIO.Platform_SetWindowTitle = SetImGuiWindowTitle;
		platformIO.Platform_RenderWindow = Unused_RenderImGuiWindow;
		platformIO.Platform_SwapBuffers = Unused_ImGuiWindowSwapBuffers;
	}

	bool ImGuiPlatform::IsWindowInContext(Window& window)
	{
		return m_windowToContextMap.contains(&window);
	}

	void ImGuiPlatform::InitializeMonitors()
	{
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		platformIO.Monitors.resize(0);

		const auto& monitors = WindowManager::Get().GetMonitors();
	
		for (auto monitor : monitors)
		{
			ImGuiPlatformMonitor imguiMonitor;
		
			const glm::vec2& monitorPos = monitor->GetMonitorPos();
			const glm::vec2& monitorSize = monitor->GetMonitorSize();
			const glm::vec2& monitorWorkPos = monitor->GetMonitorWorkPos();
			const glm::vec2& monitorWorkSize = monitor->GetMonitorWorkSize();
			const glm::vec2& contentScale = monitor->GetContentScale();

			imguiMonitor.MainPos = { monitorPos.x, monitorPos.y };
			imguiMonitor.MainSize = { monitorSize.x, monitorSize.y };
			imguiMonitor.WorkPos = { monitorWorkPos.x, monitorWorkPos.y };
			imguiMonitor.WorkSize = { monitorWorkSize.x, monitorWorkSize.y };
			imguiMonitor.DpiScale = contentScale.x;
			imguiMonitor.PlatformHandle = monitor.get();

			platformIO.Monitors.push_back(imguiMonitor);
		}
	}

	void ImGuiPlatform::UpdateMouseData()
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
	
		ImGuiID mouseHoveredViewportId = 0;
		for (int32_t i = 0; i < platformIO.Viewports.Size; ++i)
		{
			ImGuiViewport* viewport = platformIO.Viewports[i];
			Window* window = reinterpret_cast<Window*>(viewport->PlatformHandle);

			if (window == nullptr)
			{
				continue;
			}

			ContextData& contextData = m_windowToContextMap.at(window);

			if (window->IsFocused())
			{
				if (contextData.mouseWindow == nullptr)
				{
					auto [cursorX, cursorY] = window->GetCursorPos();
				
					if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
					{
						auto [windowX, windowY] = window->GetPosition();
						cursorX += windowX;
						cursorY += windowY;
					}

					contextData.lastValidMousePos = { cursorX, cursorY };
					io.AddMousePosEvent(cursorX, cursorY);
				}
			}

			const bool windowNoInput = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
			window->EnableMousePassthrough(windowNoInput);

			if (window->IsHovered())
			{
				mouseHoveredViewportId = viewport->ID;
			}
		}

		if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport)
		{
			io.AddMouseViewportEvent(mouseHoveredViewportId);
		}
	}

	void ImGuiPlatform::UpdateMouseCursor()
	{
		Window& mainWindow = WindowManager::Get().GetMainWindow();

		ImGuiIO& io = ImGui::GetIO();
		if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || !mainWindow.IsCursorEnabled())
		{
			return;
		}

		ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
		ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
		for (int32_t i = 0; i < platformIO.Viewports.Size; ++i)
		{
			ImGuiViewport* viewport = platformIO.Viewports[i];
			Window* window = reinterpret_cast<Window*>(viewport->PlatformHandle);

			if (window == nullptr)
			{
				continue;
			}

			if (imguiCursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
			{
				window->ShowCursor(false);
			}
			else
			{
				switch (imguiCursor)
				{
					case ImGuiMouseCursor_Arrow: window->SetCursor(CursorType::Arrow); break;
					case ImGuiMouseCursor_TextInput: window->SetCursor(CursorType::TextInput); break;
					case ImGuiMouseCursor_ResizeAll: window->SetCursor(CursorType::ResizeAll); break;
					case ImGuiMouseCursor_ResizeNS: window->SetCursor(CursorType::ResizeNS); break;
					case ImGuiMouseCursor_ResizeEW: window->SetCursor(CursorType::ResizeEW); break;
					case ImGuiMouseCursor_ResizeNESW: window->SetCursor(CursorType::ResizeNESW); break;
					case ImGuiMouseCursor_ResizeNWSE: window->SetCursor(CursorType::ResizeNWSE); break;
					case ImGuiMouseCursor_Hand: window->SetCursor(CursorType::Hand); break;
					case ImGuiMouseCursor_NotAllowed: window->SetCursor(CursorType::NotAllowed); break;

					default:
						window->SetCursor(CursorType::Arrow); break;
				}

				window->ShowCursor(true);
			}
		}
	}

	ImGuiPlatform::ContextData& ImGuiPlatform::GetContextDataFromWindow(Window& window)
	{
		return m_windowToContextMap.at(&window);
	}

	ImGuiContext* ImGuiPlatform::GetImGuiContextFromWindow(Window& window)
	{
		return m_windowToContextMap.at(&window).imguiContext;
	}
}
