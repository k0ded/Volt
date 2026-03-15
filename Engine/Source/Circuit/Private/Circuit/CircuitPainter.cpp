#include "circuitpch.h"
#include "CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"

#include <Volt-Assets/Font.h>
#include <Volt-Assets/MSDFData.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <CoreUtilities/StringUtility.h>

namespace Circuit
{
	glm::vec2 CircuitPainter::GetAllotedSize() const
	{
		return m_allottedScreenArea.GetSize();
	}

	void CircuitPainter::AddWidget(Ref<Widget> widget, const Volt::Rect& allotedLocalArea)
	{
		Volt::Rect subAllotedScreenArea = Volt::Rect(m_allottedScreenArea.GetPosition() + allotedLocalArea.GetPosition(), allotedLocalArea.GetSize());
		CircuitPainter subPainter = CreateSubPainter(subAllotedScreenArea);
		widget->OnPaint(subPainter);

		std::vector<CircuitDrawCommand> commands = subPainter.GetCommands();

		Volt::Rect bounds = Volt::Rect(allotedLocalArea.GetPosition(), glm::vec2(0.f, 0.f));

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

		//bounds are local here, transform them into screen bounds
		bounds.SetPosition(bounds.GetPosition() + m_basePainter->m_allottedScreenArea.GetPosition());

		widget->SetBounds(bounds);
		widget->SetAllotedScreenArea(subAllotedScreenArea);
	}

	void CircuitPainter::AddRect(float x, float y, float width, float height, CircuitColor color, float rotation, float scale)
	{
		CircuitDrawCommand command;
		command.type = CircuitPrimitiveType::Rect;

		command.pixelPos = ToPixelPos({ x,y });

		command.radiusHalfSize.x = width / 2;
		command.radiusHalfSize.y = height / 2;

		command.rotation = rotation;

		command.scale = scale;

		command.color = color;

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddCircle(float x, float y, float radius, CircuitColor color, float scale)
	{
		CircuitDrawCommand command;
		command.type = CircuitPrimitiveType::Circle;
		command.pixelPos = ToPixelPos({ x,y });

		command.radiusHalfSize.x = radius;

		command.scale = scale;

		command.color = color;


		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddText(float inX, float inY, const std::string& text, Ref<Volt::Font> font, float maxWidth, CircuitColor color, float scale)
	{
		const glm::vec2 pixelPos = ToPixelPos({ inX, inY });
		const float x = pixelPos.x;
		const float y = pixelPos.y;

		if (text.empty())
		{
			return;
		}

		std::u32string utf32string = ::Utility::To_UTF32(text);

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


				CircuitDrawCommand command;
				command.type = CircuitPrimitiveType::TextCharacter;
				command.pixelPos.x = x;
				command.pixelPos.y = y;
				command.color = color;
				command.scale = scale;

				command.minMaxPx.x = static_cast<float>(pl) * scale + x;
				command.minMaxPx.y = static_cast<float>(pb) * scale + y;
				command.minMaxPx.z = static_cast<float>(pr) * scale + x;
				command.minMaxPx.w = static_cast<float>(pt) * scale + y;

				command.minMaxUV.x = static_cast<float>(l);
				command.minMaxUV.y = static_cast<float>(b);
				command.minMaxUV.z = static_cast<float>(r);
				command.minMaxUV.w = static_cast<float>(t);

				AddDrawCommand(std::move(command));

				//cmd.texture = font->GetAtlas()->GetImage()->GetHandle<Volt::ResourceHandle>(); TODO: BROKEN

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
	glm::vec2 CircuitPainter::ToPixelPos(const glm::vec2& localPos)
	{
		return localPos + m_allottedScreenArea.GetPosition() - m_basePainter->m_allottedScreenArea.GetPosition();
	}
	void CircuitPainter::AddDrawCommand(CircuitDrawCommand&& command)
	{
		//std::vector<CircuitDrawCommand>* drawCommandsToAppendTo = &m_drawCommands;
		//if (m_basePainter)
		//{
		//	drawCommandsToAppendTo = &m_basePainter->m_drawCommands;
		//}
		//drawCommandsToAppendTo->push_back(command);

		m_drawCommands.push_back(CircuitDrawCommand(command));
		if (m_basePainter != this)
		{
			m_basePainter->m_drawCommands.push_back(command);
		}
	}

}
