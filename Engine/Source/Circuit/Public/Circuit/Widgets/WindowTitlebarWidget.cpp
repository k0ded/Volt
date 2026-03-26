#include "circuitpch.h"
#include "WindowTitlebarWidget.h"

#include "Circuit/Widgets/Layout/LayoutWidget.h"
#include "Circuit/Widgets/ButtonWidget.h"
#include "Circuit/CircuitPainter.h"

#include "TextWidget.h"


namespace Circuit
{
	glm::vec2 WindowTitlebarWidget::GetDesiredSize()
	{
		return { -1, m_height };
	}
	void WindowTitlebarWidget::Build(const Arguments& args)
	{
		m_height = args._Height;
		m_iconSize = args._IconSize;

		m_titlebarColor = args._Color;

		m_onRequestClose = args._OnRequestClose;
		m_onRequestMinimize = args._OnRequestMinimize;
		m_onRequestMaximize = args._OnRequestMaximize;

		Ref<LayoutWidget> layout = CreateWidget(LayoutWidget).Orientation(LayoutOrientation::Horizontal);

		layout->AddSpring();
		layout->AddFixedSlice(CreateMinimizeButton(), 50, 2);
		layout->AddFixedSlice(CreateMaximizeButton(), 50, 2);
		layout->AddFixedSlice(CreateCloseButton(), 50, 2);

		AddChildWidget(layout);
	}

	void WindowTitlebarWidget::OnPaint(CircuitPainter& painter)
	{
		painter.AddRect(0, 0, painter.GetAllottedSize().x, painter.GetAllottedSize().y, m_titlebarColor);

		//volt icon
		const CircuitColor White = 0xffffffff;

		const float iconRadius = m_iconSize / 2;
		constexpr float iconBorderThickness = 1.5f;
		painter.AddCircle(iconRadius, iconRadius, iconRadius, White);
		painter.AddCircle(iconRadius, iconRadius, iconRadius - iconBorderThickness * 2, m_titlebarColor);

		const float mainBoltHeight = m_iconSize * 0.75f;
		const float mainBoltThickness = 6;
		painter.AddRect(iconRadius, 0, mainBoltThickness, mainBoltHeight, White);

		const float sideSparksThickness = 3;
		const float sideSparksLength = m_iconSize * 0.35f;
		painter.AddRect(iconRadius + mainBoltThickness, m_iconSize * 0.3f, sideSparksLength, sideSparksThickness, White);
		painter.AddRect(iconRadius + mainBoltThickness - sideSparksLength, m_iconSize * 0.4f, sideSparksLength, sideSparksThickness, White);
		painter.AddRect(iconRadius + mainBoltThickness, m_iconSize * 0.6f, sideSparksLength, sideSparksThickness, White);


		CompoundWidget::OnPaint(painter);
	}

	void WindowTitlebarWidget::OnBeginHover(const WidgetInteractionData& interactionData)
	{
		m_titlebarHovered = true;
	}

	void WindowTitlebarWidget::OnEndHover(const WidgetInteractionData& interactionData)
	{
		m_titlebarHovered = false;
	}

	Ref<Widget> WindowTitlebarWidget::CreateMinimizeButton()
	{
		return CreateWidget(ButtonWidget)
			.Text("-")
			.OnReleased_Raw(this, &WindowTitlebarWidget::OnMinimizeButtonReleased);
	}
	Ref<Widget> WindowTitlebarWidget::CreateMaximizeButton()
	{
		return CreateWidget(ButtonWidget)
			.Text("=")
			.OnReleased_Raw(this, &WindowTitlebarWidget::OnMaximizeButtonReleased);
	}
	Ref<Widget> WindowTitlebarWidget::CreateCloseButton()
	{
		return CreateWidget(ButtonWidget)
			.Text("x")
			.OnReleased_Raw(this, &WindowTitlebarWidget::OnCloseButtonReleased);
	}

	void WindowTitlebarWidget::OnCloseButtonReleased(Volt::InputCode mouseButton)
	{
		m_onRequestClose.ExecuteIfBound();
	}
	void WindowTitlebarWidget::OnMinimizeButtonReleased(Volt::InputCode mouseButton)
	{
		m_onRequestMinimize.ExecuteIfBound();
	}
	void WindowTitlebarWidget::OnMaximizeButtonReleased(Volt::InputCode mouseButton)
	{
		m_onRequestMaximize .ExecuteIfBound();
	}
}
