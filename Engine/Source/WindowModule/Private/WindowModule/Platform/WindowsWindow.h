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

		void Close() override;
		void Render() override;

		void SetTitle(const WString& title) override;

		void SetPositionX(int32_t xPos) override;
		void SetPositionY(int32_t yPos) override;

		const WString& GetTitle() const override;

		uint32_t GetWidth() const override;
		uint32_t GetHeight() const override;
		int32_t GetPositionX() const override;
		int32_t GetPositionY() const override;
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

		OnWindowClosed& GetOnWindowClosed() override;
		OnWindowRepaint& GetOnWindowRepaint() override;
		OnWindowRender& GetOnWindowRender() override;
		OnWindowResize& GetOnWindowResize() override;

		IsHoveringTitlebar& GetIsHoveringTitlebar() override;
		IsHoveringMaximizeButton& GetIsHoveringMaximizeButton() override;

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

		LRESULT HandleUndecoratedWindowMessages(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

		void CreateSwapchain();

		IsHoveringTitlebar m_isHoveringTitlebar;
		IsHoveringMaximizeButton m_isHoveringMaximizeButton;

		OnWindowClosed m_onWindowClosed;
		OnWindowRepaint m_onWindowRepaint;
		OnWindowRender m_onWindowRender;
		OnWindowResize m_onWindowResize;

		WindowsWindowInputManager m_inputManager;
		PlatformWindowHandle m_nativeHandle;

		IntRef<RHI::Swapchain> m_swapchain;

		WString m_title;

		WindowHandle m_handle;

		int32_t m_posX;
		int32_t m_posY;

		uint32_t m_width;
		uint32_t m_height;

		bool m_enableVSync : 1;
		bool m_isDecorated : 1;
	};
}
