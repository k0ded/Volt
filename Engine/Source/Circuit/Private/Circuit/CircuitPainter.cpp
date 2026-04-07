#include "circuitpch.h"
#include "CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"

#include <Volt-Renderer/Texture/Texture2D.h>

namespace Circuit
{
	glm::vec2 CircuitPainter::GetAllottedSize() const
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

				case CircuitPrimitiveType::Image:
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

		command.position += command.halfSize;

		command.bounds = 
		{ 
			command.position - command.halfSize,
			command.position + command.halfSize 
		};

		command.color = color;

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddRectOutline(float x, float y, float width, float height, CircuitColor color, float lineThickness, float rotation, float scale)
	{
		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Rect;

		command.position = ToPixelPos({ x,y });
		command.rotation = rotation;
		command.scale = scale;

		command.halfSize.x = width / 2;
		command.halfSize.y = height / 2;

		command.position += command.halfSize;

		command.bounds =
		{
			command.position - command.halfSize,
			command.position + command.halfSize
		};

		command.radiusInner = lineThickness;

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

		command.bounds =
		{
			command.position - radius,
			command.position + radius
		};

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

		command.bounds =
		{
			command.position - outerRadius,
			command.position + outerRadius
		};

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

		command.bounds =
		{
			glm::min(command.lineA, command.lineB),
			glm::max(command.lineA, command.lineB)
		};

		AddDrawCommand(std::move(command));
	}

	void CircuitPainter::AddText(float inX, float inY, const String& text, AssetReference<Volt::FontAsset> font, float maxWidth, CircuitColor color, float scale)
	{
		using namespace Volt;

		const glm::vec2 pixelPos = ToPixelPos({ inX, inY });
		const float x = pixelPos.x;
		const float y = pixelPos.y;

		if (text.empty())
		{
			return;
		}

		U32String utf32string(U32String::CtorConvert(), text);

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

			const uint32_t textureIndex = m_resourceTable->GetOrAddTextureSlotIndex(font->GetAtlas());
			const glm::uvec2 dimensions = { font->GetAtlas()->GetWidth(), font->GetAtlas()->GetHeight() };

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
				
				command.textureIndex = textureIndex;
				command.dimensions = dimensions;

				command.bounds = {
					command.minMaxPx.x, command.minMaxPx.w,
					command.minMaxPx.z, command.minMaxPx.y
				};

				AddDrawCommand(std::move(command));

				double advance = glyph->GetAdvance();
				fontGeometry.GetAdvance(advance, character, utf32string[i + 1]);
				sX += fsScale * advance;
			}
		}
	}

	void CircuitPainter::AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float scale /*= 1.f*/)
	{
		AddImage(x, y, width, height, image, 0.f, 0.f, 1.f, 1.f, scale);
	}

	void CircuitPainter::AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float uv0x, float uv0y, float uv1x, float uv1y, float scale /*= 1.f*/)
	{
		const glm::vec2 pixelPos = ToPixelPos({ x, y });

		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Image;
		command.position.x = pixelPos.x;
		command.position.y = pixelPos.y;
		command.scale = scale;

		//const float firstHalfWidth = glm::floor(width * 0.5f);
		//const float firstHalfHeight = glm::floor(height * 0.5f);
		//const float secondHalfWidth = glm::ceil(width * 0.5f);
		//const float secondHalfHeight = glm::ceil(height * 0.5f);

		command.minMaxPx.x = pixelPos.x;
		command.minMaxPx.y = pixelPos.y;
		command.minMaxPx.z = pixelPos.x + width;
		command.minMaxPx.w = pixelPos.y + height;
	
		command.minMaxUV.x = uv0x;
		command.minMaxUV.y = uv0y;
		command.minMaxUV.z = uv1x;
		command.minMaxUV.w = uv1y;

		command.bounds = command.minMaxPx;

		command.textureIndex = m_resourceTable->GetOrAddTextureSlotIndex(image);
		command.dimensions = { image->GetWidth(), image->GetHeight() };

		AddDrawCommand(std::move(command));
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

		//TODO: extremely wasteful to add to 3 different lists, need to refactor this whole paiting system
		m_drawCommands.push_back(CircuitDrawCommand(command));
		if (m_basePainter != this)
		{
			m_basePainter->m_drawCommands.push_back(command);
		}
		if (m_parentPainter)
		{
			m_parentPainter->m_drawCommands.push_back(command);
		}
	}

}
