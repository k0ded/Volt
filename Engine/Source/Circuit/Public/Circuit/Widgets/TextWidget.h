#pragma once
#include "Circuit/Widgets/Widget.h"
#include "Circuit/CircuitColor.h"

namespace Volt
{
	class Font;
}

namespace Circuit
{
	class CIRCUIT_API TextWidget : public Widget
	{
	public:
		TextWidget();
		virtual ~TextWidget();

		CIRCUIT_BEGIN_ARGS(TextWidget): 
			_Size(21.f),
			_Color(0xffffffff)
		{
		};
		CIRCUIT_ATTRIBUTE(std::string, Text);
		CIRCUIT_ATTRIBUTE(CircuitColor, Color);
		CIRCUIT_ARGUMENT(float, Size);

		CIRCUIT_END_ARGS();

		void Build(const Arguments& args);

		virtual glm::vec2 OnLayout(const glm::vec2& allotedSize) override;
		virtual void OnPaint(CircuitPainter& painter) override;

		virtual bool IsHittestInvisible() const { return true; };
	private:
		Volt::Attribute<std::string> m_text;
		Volt::Attribute<CircuitColor> m_color;
		float m_size;

		Ref<Volt::Font> m_font;
	};
}
