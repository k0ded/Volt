#include "circuitpch.h"
#include "CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"

#include <Volt-Assets/Assets/Font.h>
#include <Volt-Assets/Assets/MSDFData.h>

#include <CoreUtilities/StringUtility.h>

namespace Circuit
{

	const Volt::Rect& CircuitPainter::GetAllotedArea() const
	{
		return m_allottedArea;
	}

	void CircuitPainter::AddWidget(Ref<Widget> widget, const Volt::Rect& allotedArea)
	{
		Volt::Rect subAllotedArea = Volt::Rect(allotedArea.GetPosition() + m_allottedArea.GetPosition(), allotedArea.GetSize());
		CircuitPainter subPainter = CreateSubPainter(subAllotedArea);
		widget->OnPaint(subPainter);

		if (m_calculateBounds)
		{
			std::vector<CircuitDrawCommand> commands = subPainter.GetCommands();

			Volt::Rect bounds = Volt::Rect(allotedArea.GetPosition(), glm::vec2(0.f, 0.f));

			for (CircuitDrawCommand& command : commands)
			{
				switch (command.type)
				{
					case CircuitPrimitiveType::Rect:
						bounds.MergeRectIntoThis(Volt::Rect(command.pixelPos, command.radiusHalfSize * 2.f));
						break;

					case CircuitPrimitiveType::Circle:
						bounds.MergeRectIntoThis(Volt::Rect(command.pixelPos - glm::vec2(command.radiusHalfSize.x), command.radiusHalfSize.x * 2.f));
						break;
					case CircuitPrimitiveType::TextCharacter:
						bounds.MergeRectIntoThis(Volt::Rect(command.minMaxPx.x, command.minMaxPx.y, glm::abs(command.minMaxPx.z - command.minMaxPx.x), glm::abs(command.minMaxPx.w - command.minMaxPx.y)));
						break;
				}
			}

			//widget->SetBounds(bounds);
		}
	}

	void CircuitPainter::AddRect(float x, float y, float width, float height, CircuitColor color, float rotation, float scale)
	{
		CircuitDrawCommand command;
		command.type = CircuitPrimitiveType::Rect;

		command.pixelPos.x = x + m_allottedArea.GetPosition().x;
		command.pixelPos.y = y + m_allottedArea.GetPosition().y;

		command.radiusHalfSize.x = width / 2;
		command.radiusHalfSize.y = height / 2;

		command.rotation = rotation;

		command.scale = scale;

		command.color = color;

		std::vector<CircuitDrawCommand>* drawCommandsToAppendTo = &m_drawCommands;
		if (m_basePainter)
		{
			drawCommandsToAppendTo = &m_basePainter->m_drawCommands;
		}
		drawCommandsToAppendTo->push_back(command);
	}

	void CircuitPainter::AddCircle(float x, float y, float radius, CircuitColor color, float scale)
	{
		CircuitDrawCommand command;
		command.type = CircuitPrimitiveType::Circle;
		command.pixelPos.x = x + m_allottedArea.GetPosition().x;
		command.pixelPos.y = y + m_allottedArea.GetPosition().y;

		command.radiusHalfSize.x = radius;

		command.scale = scale;

		command.color = color;


		std::vector<CircuitDrawCommand>* drawCommandsToAppendTo = &m_drawCommands;
		if (m_basePainter)
		{
			drawCommandsToAppendTo = &m_basePainter->m_drawCommands;
		}
		drawCommandsToAppendTo->push_back(command);
	}

	void CircuitPainter::AddText(float inX, float inY, const std::string& text, Ref<Volt::Font> font, float maxWidth, CircuitColor color, float scale)
	{
		const float x = inX +m_allottedArea.GetPosition().x;
		const float y = inY +m_allottedArea.GetPosition().y;

		if (text.empty())
		{
			return;
		}
		
		std::u32string utf32string = ::Utility::ToUTF32(text);

		auto& fontGeom = font->GetMSDFData()->fontGeometry;
		const auto& metrics = fontGeom.getMetrics();

		Vector<int32_t> lineSplits;

		// Find all line splits
		{
			double sX = 0.0;
			double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
			double sY = -fsScale * metrics.ascenderY;

			int32_t lastSpace = -1;

			for (int32_t i = 0; i < static_cast<int32_t>(utf32string.size()); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					sX = 0.0;
					sY -= fsScale * metrics.lineHeight;
					continue;
				}

				auto glyph = fontGeom.getGlyph(character);
				if (!glyph)
				{
					glyph = fontGeom.getGlyph('?');
				}

				VT_ENSURE(glyph);

				if (character != ' ')
				{
					double pl, pb, pr, pt;
					glyph->getQuadPlaneBounds(pl, pb, pr, pt);
					glm::vec2 quadMin((float)pl, (float)pb);
					glm::vec2 quadMax((float)pl, (float)pb);

					quadMin *= (float)fsScale;
					quadMax *= (float)fsScale;
					quadMin += glm::vec2((float)sX, (float)sY);
					quadMax += glm::vec2((float)sX, (float)sY);

					if (quadMax.x > maxWidth && lastSpace != -1)
					{
						i = lastSpace;
						lineSplits.emplace_back(lastSpace);
						lastSpace = -1;
						sX = 0.0;
						sY -= fsScale * metrics.lineHeight;
					}
				}
				else
				{
					lastSpace = i;
				}

				double advance = glyph->getAdvance();
				fontGeom.getAdvance(advance, character, utf32string[i + 1]);
				sX += fsScale * advance;
			}
		}

		// Setup commands
		{
			double sX = 0.0;
			double fsScale = 1.0 / (metrics.ascenderY - metrics.descenderY);
			double sY = -fsScale * metrics.ascenderY;

			for (int32_t i = 0; i < static_cast<int32_t>(utf32string.size()); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					sX = 0.0;
					sY += fsScale * metrics.lineHeight;
					continue;
				}

				auto glyph = fontGeom.getGlyph(character);
				if (!glyph)
				{
					glyph = fontGeom.getGlyph('?');
				}

				VT_ENSURE(glyph);

				double l, b, r, t;
				glyph->getQuadAtlasBounds(l, b, r, t);

				double pl, pb, pr, pt;
				glyph->getQuadPlaneBounds(pl, pb, pr, pt);

				pt = metrics.ascenderY - pt;
				pb = metrics.ascenderY - pb;

				pt += metrics.ascenderY + metrics.descenderY;
				pb += metrics.ascenderY + metrics.descenderY;

				pl *= fsScale, pb *= fsScale, pr *= fsScale, pt *= fsScale;
				pl += sX, pb += sY, pr += sX, pt += sY;

				double texelWidth = 1.0 / font->GetAtlas()->GetWidth();
				double texelHeight = 1.0 / font->GetAtlas()->GetHeight();

				l *= texelWidth, b *= texelHeight, r *= texelWidth, t *= texelHeight;

				std::vector<CircuitDrawCommand>* drawCommandsToAppendTo = &m_drawCommands;
				if (m_basePainter)
				{
					drawCommandsToAppendTo = &m_basePainter->m_drawCommands;
				}

				CircuitDrawCommand& cmd = drawCommandsToAppendTo->emplace_back();
				cmd.type = CircuitPrimitiveType::TextCharacter;
				cmd.pixelPos.x = x;
				cmd.pixelPos.y = y;
				cmd.color = color;
				cmd.scale = scale;

				cmd.minMaxPx.x = static_cast<float>(pl) * scale + x;
				cmd.minMaxPx.y = static_cast<float>(pb) * scale + y;
				cmd.minMaxPx.z = static_cast<float>(pr) * scale + x;
				cmd.minMaxPx.w = static_cast<float>(pt) * scale + y;

				cmd.minMaxUV.x = static_cast<float>(l);
				cmd.minMaxUV.y = static_cast<float>(b);
				cmd.minMaxUV.z = static_cast<float>(r);
				cmd.minMaxUV.w = static_cast<float>(t);

				cmd.texture = font->GetAtlasResourceHandle();

				double advance = glyph->getAdvance();
				fontGeom.getAdvance(advance, character, utf32string[i + 1]);
				sX += fsScale * advance;
			}
		}
	}

	std::vector<CircuitDrawCommand> CircuitPainter::GetCommands()
	{
		return m_drawCommands;
	}
}
