#pragma once

#include "Circuit/Widgets/Widget.h"

#include <EventSystem/EventListener.h> 

namespace Volt
{
	class MouseMovedEvent;
	class MouseButtonPressedEvent;
	class MouseButtonReleasedEvent;
}
namespace Circuit
{
	class CIRCUIT_API ButtonWidget : public Widget
	{
	public:
		ButtonWidget();
		virtual ~ButtonWidget();

		CIRCUIT_BEGIN_ARGS(ButtonWidget)
			: _MinSize(5, 5),
			_Size(-1,-1)
		{
		};
		
		CIRCUIT_ARGUMENT(glm::vec2, MinSize)
		CIRCUIT_ARGUMENT(glm::vec2, Size)

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual void OnPaint(CircuitPainter& painter) override;


		virtual void OnBeginHover();
		virtual void OnEndHover();
		virtual void OnPressed();
		virtual void OnReleased();
	private:


		bool m_hovered;
		bool m_pressed;

		glm::vec2 m_minSize;
		glm::vec2 m_size;
	};
}
