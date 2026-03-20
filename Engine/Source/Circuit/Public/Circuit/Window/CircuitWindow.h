#pragma once

#include "Circuit/CircuitDrawCommand.h"

#include <RHIModule/Descriptors/ResourceTable.h>

#include <WindowModule/WindowHandle.h>
#include <CoreUtilities/Core.h>
#include <EventSystem/EventListener.h>
#include <vector>

namespace Volt
{
	class WindowTitlebarHittestEvent;
}

namespace Circuit
{
	class CircuitRenderer;
	class Widget;
	class WindowWidget;

	class CircuitWindow : Volt::EventListener
	{
	public:
		CIRCUIT_API CircuitWindow(Volt::WindowHandle windowHandle);
		CIRCUIT_API ~CircuitWindow();

		CIRCUIT_API Volt::WindowHandle GetWindowHandle() const;

		CIRCUIT_API glm::i32vec2 GetPosition() const;
		CIRCUIT_API glm::u32vec2 GetSize() const;
		CIRCUIT_API void Resize(const glm::vec2& size);

		CIRCUIT_API std::vector<CircuitDrawCommand> GetDrawCommands();

		//takes ownership of the widget
		CIRCUIT_API void SetWidget(Ref<WindowWidget> widget);
		CIRCUIT_API Weak<WindowWidget> GetWidget() { return m_windowWidget; };


		void OnRender();
	private:
		void RegisterEventListeners();
		bool OnWindowTitlebarHittestEvent(class Volt::WindowTitlebarHittestEvent& e);

		const Volt::WindowHandle m_windowHandle;

		Ref<CircuitRenderer> m_renderer;
		IntRef<Volt::RHI::ResourceTable> m_resourceTable;

		glm::u16vec2 m_windowSize;
		std::string m_title;

		Ref<WindowWidget> m_windowWidget;

	};
}
