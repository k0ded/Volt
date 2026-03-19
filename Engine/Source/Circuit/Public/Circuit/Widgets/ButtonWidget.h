#pragma once

#include "Circuit/Widgets/CompoundWidget.h"
#include <InputModule/InputCodes.h>

//TODO: REMOVE THIS
#include <Volt-Assets/FontAsset.h>
#include <AssetSystem/AssetReference.h>

namespace Circuit
{
	DECLARE_DELEGATE_OneParam(OnClickDelegate, Volt::InputCode /*MouseButton*/);
	class CIRCUIT_API ButtonWidget : public CompoundWidget
	{
	public:
		ButtonWidget();
		virtual ~ButtonWidget();

		CIRCUIT_BEGIN_ARGS(ButtonWidget)
			: _MinSize(5, 5)
		{
		};

		CIRCUIT_ARGUMENT(glm::vec2, MinSize);

		CIRCUIT_ARGUMENT(Ref<Widget>, Content);

		CIRCUIT_ATTRIBUTE(std::string, Text);

		CIRCUIT_EVENT(OnClickDelegate, OnPressed);
		CIRCUIT_EVENT(OnClickDelegate, OnReleased);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual glm::vec2 GetDesiredSize() override;

		virtual void OnPaint(CircuitPainter& painter) override;


		virtual void OnBeginHover(const WidgetInteractionData& interactionData) override;
		virtual void OnEndHover(const WidgetInteractionData& interactionData) override;
		virtual void OnPressed(const WidgetInteractionData& interactionData) override;
		virtual void OnReleased(const WidgetInteractionData& interactionData) override;
	private:
		OnClickDelegate m_onPressedDelegate;
		OnClickDelegate m_onReleasedDelegate;

		AssetReference<Volt::FontAsset> m_fontAsset;

		Volt::Attribute<std::string> m_text;

		Ref<Widget> m_content;

		bool m_hovered;
		bool m_pressed;

		glm::vec2 m_minSize;

		glm::vec2 m_size;
	};
}
