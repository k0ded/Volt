#pragma once
#include "Circuit/CircuitDrawCommand.h"

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Math/2DShapes/Rect.h>

namespace Volt
{
	class Font;
}

namespace Circuit
{
	class Widget;
	class CircuitPainter
	{
	public:
		CircuitPainter(const Volt::Rect& allotedScreenArea)
			: m_allottedScreenArea(allotedScreenArea), m_basePainter(this)
		{};


		~CircuitPainter() = default;

		glm::vec2 GetAllotedSize() const;

		VT_INLINE void AddWidget(Ref<Widget> widget, float x, float y, float width, float height) { AddWidget(widget,Volt::Rect(x, y, width, height)); }
		VT_INLINE void AddWidget(Ref<Widget> widget, const glm::vec2& position, const glm::vec2& size){AddWidget(widget, Volt::Rect(position, size));}
		void AddWidget(Ref<Widget> widget, const Volt::Rect& allotedArea);

		void AddRect(float x, float y, float width, float height, CircuitColor color, float rotation = 0, float scale = 1);
		void AddCircle(float x, float y, float radius, CircuitColor color, float scale = 1);
		void AddText(float x, float y, const std::string& text, Ref<Volt::Font> font, float maxWidth, CircuitColor color, float scale = 1.f);

		std::vector<CircuitDrawCommand> GetCommands();
	private:
		CircuitPainter(CircuitPainter* basePainter, const Volt::Rect& allotedScreenArea) :
			m_allottedScreenArea(allotedScreenArea),
			m_basePainter(basePainter)
		{};
		VT_INLINE CircuitPainter CreateSubPainter(const Volt::Rect& allotedScreenArea){	return CircuitPainter(m_basePainter ? m_basePainter : this, allotedScreenArea); }

		glm::vec2 ToPixelPos(const glm::vec2& localPos);
		void AddDrawCommand(CircuitDrawCommand&& command);

		std::vector<CircuitDrawCommand> m_drawCommands;

		Volt::Rect m_allottedScreenArea;

		CircuitPainter* m_basePainter = nullptr;

		bool m_calculateBounds = false;

	};
}
