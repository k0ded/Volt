#pragma once

#include "WindowModule/Window_New.h"
#include "WindowModule/Platform/WindowsWindowInputManager.h"

#include <CoreUtilities/Platform/Windows/VoltWindows.h>

#include <RHIModule/Graphics/Swapchain.h>

namespace Volt
{
	class WindowsWindow : public Window_New
	{
	public:
		WindowsWindow(const WindowInitializer& initializer, WindowHandle handle);
		~WindowsWindow() override;

		void ProcessMessages() override;
		void BeginFrame() override;
		void Present() override;

		void SetTitle(const WString& title) override;

		void SetXPosition(int32_t xPos) override;
		void SetYPosition(int32_t yPos) override;

		const WString& GetTitle() const override;

		int32_t GetWidth() const override;
		int32_t GetHeight() const override;
		WindowHandle GetHandle() const override;

		void Maximize() override;
		void Minimize() override;
		void Restore() override;
		void Focus() override;

		bool IsMaximized() const override;
		bool IsMinimized() const override;
		bool IsFocused() const override;

		WindowInputManager& GetInputManager() override;
		const RHI::Swapchain& GetSwapchain() const override;

	private:
		// The shared Windows window 'class' which describes
		// how a window looks.
		class WindowClass
		{
		public:
			static const wchar_t* GetName() noexcept;
			static HINSTANCE GetInstance() noexcept;

			static WindowClass& Get();
		private:
			WindowClass() noexcept;
			~WindowClass();

			WindowClass(const WindowClass&) = delete;
			WindowClass& operator=(const WindowClass&) = delete;
		
			const wchar_t* m_className = L"VoltWindow";
			HINSTANCE m_instance = nullptr;
		};

		static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

		void CreateSwapchain();

		WindowsWindowInputManager m_inputManager;
		PlatformWindowHandle m_nativeHandle;

		IntRef<RHI::Swapchain> m_swapchain;

		WString m_title;

		WindowHandle m_handle;

		int32_t m_posX;
		int32_t m_posY;

		int32_t m_width;
		int32_t m_height;

		bool m_enableVSync;
	};
}
