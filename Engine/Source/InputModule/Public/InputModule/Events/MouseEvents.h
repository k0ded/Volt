#pragma once

#include "InputModule/InputModuleConfig.h"
#include "InputModule/InputCodes.h"

#include <EventSystem/Event.h>

namespace Volt
{
	class Window;

	//used by the InputModule to tell the window module to set the mouse position
	class INPUTMODULE_API SetMousePositionEvent : public Event
	{
	public:
		SetMousePositionEvent(float x, float y)
			: m_mouseX(x), m_mouseY(y)
		{
		}

		//Getting
		inline float GetX() const { return m_mouseX; }
		inline float GetY() const { return m_mouseY; }

		EVENT_CLASS(MouseMovedEvent, "{60D2FCD3-ED9E-4D47-8CED-C963ABF081FA}"_guid)

	private:
		float m_mouseX;
		float m_mouseY;
	};

	//used by the InputModule to tell the window module to show or hide the cursor
	class INPUTMODULE_API SetShowCursorEvent : public Event
	{
	public:
		SetShowCursorEvent(bool show)
			: m_show(show)
		{
		}

		//Getting
		inline float ShouldShow() const { return m_show; }

		EVENT_CLASS(MouseMovedEvent, "{5B777B31-1994-4A34-A97F-422BE1A9013C}"_guid)

	private:
		bool m_show;
	};

	class INPUTMODULE_API MouseEvent : public Event
	{
	public:
		VT_NODISCARD VT_INLINE Window& GetWindow() { return m_window; }

	protected:
		MouseEvent(Window& window)
			: m_window(window)
		{ }

		Window& m_window;
	};

	class INPUTMODULE_API MouseMovedEvent : public MouseEvent
	{
	public:
		MouseMovedEvent(Window& window, float x, float y)
			: MouseEvent(window), m_mouseX(x), m_mouseY(y)
		{
		}

		//Getting
		inline float GetX() const { return m_mouseX; }
		inline float GetY() const { return m_mouseY; }

		std::string ToString() const override;

		EVENT_CLASS(MouseMovedEvent, "{7D80A1B6-FEE2-42AA-9004-AFB99EEB7E30}"_guid)

	private:
		float m_mouseX;
		float m_mouseY;
	};

	class INPUTMODULE_API MouseScrolledEvent : public MouseEvent
	{
	public:
		MouseScrolledEvent(Window& window, float xOffset, float yOffset)
			: MouseEvent(window), m_xOffset(xOffset), m_yOffset(yOffset)
		{
		}

		inline const float GetXOffset() const { return m_xOffset; }
		inline const float GetYOffset() const { return m_yOffset; }

		EVENT_CLASS(MouseScrolledEvent, "{128BF64B-5D6C-487F-9E75-DCAD110F220F}"_guid)
	private:
		float m_xOffset;
		float m_yOffset;
	};

	class INPUTMODULE_API MouseButtonEvent : public MouseEvent
	{
	public:
		//Getting
		VT_NODISCARD VT_INLINE InputCode GetMouseButton() const { return m_button; }
		VT_NODISCARD VT_INLINE InputModifier GetModifiers() const { return m_modifiers; }

		EVENT_CLASS(MouseButtonEvent, "{6A5764D2-B47E-41BF-9CA9-609AFC3610F2}"_guid)
	protected:
		MouseButtonEvent(Window& window, int32_t button, int32_t modifiers)
			: MouseEvent(window)
		{
			m_button = GLFWMouseCodeToInputCode(button);
			m_modifiers = GLFWModifierToInputModifier(modifiers);
		}

		InputCode m_button;
		InputModifier m_modifiers;
	};

	class INPUTMODULE_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(Window& window, int32_t button, int32_t modifiers)
			: MouseButtonEvent(window, button, modifiers)
		{
		}

		std::string ToString() const override;

		EVENT_CLASS(MouseButtonEvent, "{86C684FF-C169-437E-BDAC-1AF7C721C8CA}"_guid)
	};

	class INPUTMODULE_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(Window& window, int32_t button, int32_t modifiers)
			: MouseButtonEvent(window, button, modifiers)
		{
		}

		std::string ToString() const override;

		EVENT_CLASS(MouseButtonEvent, "{39C7FC4A-705C-4431-A278-811DF01773EB}"_guid)

	};


}
