#include "circuitpch.h"
#include "CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"

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
					bounds.MergeRectIntoThis(Volt::Rect(command.position, command.halfSize * 2.f));
					break;
				case CircuitPrimitiveType::CircleSegment:
				case CircuitPrimitiveType::Circle:
					bounds.MergeRectIntoThis(Volt::Rect(command.position - glm::vec2(command.radius), command.radius * 2.f));
					break;
				case CircuitPrimitiveType::TextCharacter:
					bounds.MergeRectIntoThis(Volt::Rect(command.minMaxPx.x, command.minMaxPx.y, glm::abs(command.minMaxPx.z - command.minMaxPx.x), glm::abs(command.minMaxPx.w - command.minMaxPx.y)));
					break;
				case CircuitPrimitiveType::Line:
					const glm::vec2 minPos = glm::min(command.lineA, command.lineB);
					const glm::vec2 maxPos = glm::max(command.lineA, command.lineB);
					bounds.MergeRectIntoThis(Volt::Rect(minPos, maxPos - minPos));
					break;
				default:
					VT_ENSURE_NO_ENTRY();
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
		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Rect;

		command.position = ToPixelPos({ x,y });
		command.rotation = rotation;
		command.scale = scale;

		command.halfSize.x = width / 2;
		command.halfSize.y = height / 2;

		command.color = color;

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddCircle(float x, float y, float radius, CircuitColor color, float scale)
	{
		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Circle;
		command.position = ToPixelPos({ x,y });
		command.scale = scale;

		command.color = color;

		command.radius = radius;

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddCircleSegment(float x, float y, float innerRadius, float outerRadius, float angleDegrees, CircuitColor color, float scale /*= 1*/)
	{
		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::CircleSegment;
		command.position = ToPixelPos({ x,y });
		command.scale = scale;

		command.color = color;

		command.radius = outerRadius;
		command.radiusInner = innerRadius;
		command.angle = glm::radians(angleDegrees) * 0.5f;

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddLine(float x0, float y0, float x1, float y1, float radius, CircuitColor color)
	{
		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Line;
		command.color = color;
		command.radius = radius;
		command.lineA = ToPixelPos({ x0, y0 });
		command.lineB = ToPixelPos({ x1, y1 });

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddText(float inX, float inY, const std::string& text, AssetReference<Volt::FontAsset> font, float maxWidth, CircuitColor color, float scale)
	{
		using namespace Volt;

		const glm::vec2 pixelPos = ToPixelPos({ inX, inY });
		const float x = pixelPos.x;
		const float y = pixelPos.y;

		if (text.empty())
		{
			return;
		}

		std::u32string utf32string = ::Utility::To_UTF32(text);

		const FontMetrics& fontMetrics = font->GetMetrics();
		const FontGeometry& fontGeometry = font->GetGeometry();

		GlobalMemoryStackMark memMark;
		GlobalMemoryStackVector<int32_t> lineSplits;

		// Find all line splits
		{
			double sX = 0.0;
			double fsScale = 1.0 / (fontMetrics.ascenderY - fontMetrics.descenderY);
			double sY = -fsScale * fontMetrics.ascenderY;

			int32_t lastSpace = -1;

			for (int32_t i = 0; i < static_cast<int32_t>(utf32string.size()); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					sX = 0.0;
					sY -= fsScale * fontMetrics.lineHeight;
					continue;
				}

				const GlyphGeometry* glyph = fontGeometry.GetGlyph(character);
				if (!glyph)
				{
					glyph = fontGeometry.GetGlyph('?');
				}

				VT_ENSURE(glyph);

				if (character != ' ')
				{
					const GlyphGeometry::Bounds& planeBounds = glyph->GetPlaneBounds();

					glm::vec2 quadMin((float)planeBounds.left, (float)planeBounds.bottom);
					glm::vec2 quadMax((float)planeBounds.left, (float)planeBounds.bottom);

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
						sY -= fsScale * fontMetrics.lineHeight;
					}
				}
				else
				{
					lastSpace = i;
				}

				double advance = glyph->GetAdvance();
				fontGeometry.GetAdvance(advance, character, utf32string[i + 1]);
				sX += fsScale * advance;
			}
		}

		// Setup commands
		{
			double sX = 0.0;
			double fsScale = 1.0 / (fontMetrics.ascenderY - fontMetrics.descenderY);
			double sY = -fsScale * fontMetrics.ascenderY;

			for (int32_t i = 0; i < static_cast<int32_t>(utf32string.size()); i++)
			{
				char32_t character = utf32string[i];
				if (character == '\n')
				{
					sX = 0.0;
					sY += fsScale * fontMetrics.lineHeight;
					continue;
				}

				auto glyph = fontGeometry.GetGlyph(character);
				if (!glyph)
				{
					glyph = fontGeometry.GetGlyph('?');
				}

				VT_ENSURE(glyph);

				GlyphGeometry::Bounds atlasBounds = glyph->GetAtlasBounds();
				GlyphGeometry::Bounds planeBounds = glyph->GetPlaneBounds();

				planeBounds.top = fontMetrics.ascenderY - planeBounds.top;
				planeBounds.bottom = fontMetrics.ascenderY - planeBounds.bottom;

				planeBounds.top += fontMetrics.ascenderY + fontMetrics.descenderY;
				planeBounds.bottom += fontMetrics.ascenderY + fontMetrics.descenderY;

				planeBounds.left *= fsScale; 
				planeBounds.bottom *= fsScale;
				planeBounds.right *= fsScale; 
				planeBounds.top *= fsScale;

				planeBounds.left += sX; 
				planeBounds.bottom += sY; 
				planeBounds.right += sX; 
				planeBounds.top += sY;

				double texelWidth = 1.0 / font->GetAtlas()->GetWidth();
				double texelHeight = 1.0 / font->GetAtlas()->GetHeight();

				atlasBounds.left *= texelWidth; 
				atlasBounds.bottom *= texelHeight; 
				atlasBounds.right *= texelWidth; 
				atlasBounds.top *= texelHeight;

				CircuitDrawCommand command = CircuitDrawCommand::Initialize();
				command.type = CircuitPrimitiveType::TextCharacter;
				command.position.x = x;
				command.position.y = y;
				command.color = color;
				command.scale = scale;

				command.minMaxPx.x = static_cast<float>(planeBounds.left) * scale + x;
				command.minMaxPx.y = static_cast<float>(planeBounds.bottom) * scale + y;
				command.minMaxPx.z = static_cast<float>(planeBounds.right) * scale + x;
				command.minMaxPx.w = static_cast<float>(planeBounds.top) * scale + y;

				command.minMaxUV.x = static_cast<float>(atlasBounds.left);
				command.minMaxUV.y = static_cast<float>(atlasBounds.bottom);
				command.minMaxUV.z = static_cast<float>(atlasBounds.right);
				command.minMaxUV.w = static_cast<float>(atlasBounds.top	);
				
				command.textureIndex = m_resourceTable->GetOrAddTextureSlotIndex(font->GetAtlas());

				AddDrawCommand(std::move(command));

				double advance = glyph->GetAdvance();
				fontGeometry.GetAdvance(advance, character, utf32string[i + 1]);
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
