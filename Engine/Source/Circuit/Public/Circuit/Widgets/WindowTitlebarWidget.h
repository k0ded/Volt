#pragma once
#include "Circuit/Widgets/CompoundWidget.h"

#include "Circuit/CircuitColor.h"

#include <CoreUtilities/Delegates/Delegate.h>

#include <InputModule/InputCodes.h>
namespace Circuit
{
	DECLARE_DELEGATE(OnTitlebarButtonInteracted);
	class LayoutWidget;

	class CIRCUIT_API WindowTitlebarWidget : public CompoundWidget
	{
	public:
		CIRCUIT_BEGIN_ARGS(WindowTitlebarWidget)
			: _Color(30)
			, _IconSize(40)
			, _Height(50)
		{
		};

		CIRCUIT_ARGUMENT(CircuitColor, Color);
		CIRCUIT_ARGUMENT(float, IconSize);
		CIRCUIT_ARGUMENT(float, Height);

		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestClose);
		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestMinimize);
		CIRCUIT_EVENT(OnTitlebarButtonInteracted, OnRequestMaximize);

		CIRCUIT_END_ARGS();

		virtual glm::vec2 GetDesiredSize() override;
		void Build(const Arguments& args);
		virtual void OnPaint(CircuitPainter& painter) override;

		bool IsHoveringTitlebar() const { return m_titlebarHovered; }

		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override;
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override;
	private:
		Ref<Widget> CreateMinimizeButton();
		Ref<Widget> CreateMaximizeButton();
		Ref<Widget> CreateCloseButton();

		void OnCloseButtonReleased(Volt::InputCode mouseButton);
		void OnMinimizeButtonReleased(Volt::InputCode mouseButton);
		void OnMaximizeButtonReleased(Volt::InputCode mouseButton);

		OnTitlebarButtonInteracted m_onRequestClose;
		OnTitlebarButtonInteracted m_onRequestMinimize;
		OnTitlebarButtonInteracted m_onRequestMaximize;


		float m_iconSize;
		float m_height;

		bool m_titlebarHovered = false;
		CircuitColor m_titlebarColor;
	};

}
