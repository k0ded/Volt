#include "circuitpch.h"
#include "CircuitPainter.h"

#include "Circuit/Widgets/Widget.h"
#include "Circuit/ConsoleVars.h"
#include "Circuit/LogCategories.h"

#include <Volt-Renderer/Texture/Texture2D.h>

#include <LogModule/Log.h>

namespace Circuit
{
	CircuitPainter::CircuitPainter(const glm::vec2& windowOrigin, IntRef<Volt::RHI::ResourceTable> resourceTable, PainterPool* pool)
		: m_allottedScreenArea(0.f, 0.f, 0.f, 0.f),
		m_windowOrigin(windowOrigin),
		m_resourceTable(resourceTable),
		m_pool(pool)
	{
	}

	CircuitPainter::~CircuitPainter() = default;
	CircuitPainter::CircuitPainter(CircuitPainter&&) noexcept = default;
	CircuitPainter& CircuitPainter::operator=(CircuitPainter&&) noexcept = default;

	void CircuitPainter::SetAllottedScreenArea(const Volt::Rect& allottedScreenArea)
	{
		m_allottedScreenArea = allottedScreenArea;
	}

	glm::vec2 CircuitPainter::GetAllottedSize() const
	{
		return m_allottedScreenArea.GetSize();
	}

	void CircuitPainter::AddWidget(Ref<Widget> widget, const Volt::Rect& allottedLocalArea)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(m_pool);

		if (!m_pool->Contains(widget))
		{
			VT_LOGC(Warning, LogCircuit, "[AddWidget] widget {} not pre-reserved; extending pool. Likely caused by mutating child list during OnPaint.", static_cast<const void*>(widget.GetRaw()));
			m_pool->Reserve(widget);
		}

		Volt::Rect subAllottedScreenArea = Volt::Rect(m_allottedScreenArea.GetPosition() + allottedLocalArea.GetPosition(), allottedLocalArea.GetSize());

		CircuitPainter& childPainter = m_pool->GetFor(widget);
		childPainter.SetAllottedScreenArea(subAllottedScreenArea);

		ChildSlot slot;
		slot.insertBefore = m_ownCommands.size();
		slot.widget = widget;
		slot.allottedScreenArea = subAllottedScreenArea;
		slot.painter = &childPainter;

		m_childSlots.push_back(std::move(slot));
	}

	void CircuitPainter::AddRect(float x, float y, float width, float height, CircuitColor color, float rotation, float scale)
	{
		VT_PROFILE_FUNCTION();
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
		VT_PROFILE_FUNCTION();
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
		VT_PROFILE_FUNCTION();
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
		VT_PROFILE_FUNCTION();
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
		VT_PROFILE_FUNCTION();
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
		VT_PROFILE_FUNCTION();
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

			IntRef<RHI::Image> atlas = font->GetAtlas();

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

				double texelWidth = 1.0 / atlas->GetWidth();
				double texelHeight = 1.0 / atlas->GetHeight();

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
				command.minMaxUV.w = static_cast<float>(atlasBounds.top);

				command.bounds = {
					command.minMaxPx.x, command.minMaxPx.w,
					command.minMaxPx.z, command.minMaxPx.y
				};

				AddDrawCommand(std::move(command), atlas);

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
		VT_PROFILE_FUNCTION();
		const glm::vec2 pixelPos = ToPixelPos({ x, y });

		CircuitDrawCommand command = CircuitDrawCommand::Initialize();
		command.type = CircuitPrimitiveType::Image;
		command.position.x = pixelPos.x;
		command.position.y = pixelPos.y;
		command.scale = scale;

		command.minMaxPx.x = pixelPos.x;
		command.minMaxPx.y = pixelPos.y;
		command.minMaxPx.z = pixelPos.x + width;
		command.minMaxPx.w = pixelPos.y + height;

		command.minMaxUV.x = uv0x;
		command.minMaxUV.y = uv0y;
		command.minMaxUV.z = uv1x;
		command.minMaxUV.w = uv1y;

		command.bounds = command.minMaxPx;

		AddDrawCommand(std::move(command), image);
	}

	Volt::Rect CircuitPainter::Consolidate(Vector<CircuitDrawCommand>& out)
	{
		VT_PROFILE_FUNCTION();
		const bool log = s_cvarCircuitLogPaint.GetValue() != 0;

		// BFS-collect all painters reachable from this one via child slots.
		// PainterPool guarantees parents appear before children, so this list is already
		// in topological order — reversing it gives leaves-first (children before parents).
		Vector<CircuitPainter*> painters;
		painters.push_back(this);
		for (size_t i = 0; i < painters.size(); ++i)
		{
			for (const ChildSlot& slot : painters[i]->m_childSlots)
			{
				if (slot.painter)
				{
					painters.push_back(slot.painter);
				}
			}
		}

		// Per-painter temp output buffers and bounds.
		// Process in reverse (leaves first) so a parent can splice already-built child buffers.
		Map<CircuitPainter*, Vector<CircuitDrawCommand>> perPainterOut;
		Map<CircuitPainter*, Volt::Rect> perPainterBounds;
		perPainterOut.reserve(painters.size());
		perPainterBounds.reserve(painters.size());

		for (size_t pi = painters.size(); pi-- > 0; )
		{
			CircuitPainter* p = painters[pi];
			Vector<CircuitDrawCommand>& pOut = perPainterOut[p];

			if (log)
			{
				VT_LOGC(Info, LogCircuit, "[Consolidate] enter painter={} ownCmds={} childSlots={} alloted=({},{} {}x{})",
					static_cast<const void*>(p),
					p->m_ownCommands.size(),
					p->m_childSlots.size(),
					p->m_allottedScreenArea.GetPosition().x, p->m_allottedScreenArea.GetPosition().y,
					p->m_allottedScreenArea.GetSize().x, p->m_allottedScreenArea.GetSize().y);
			}

			Volt::Rect bounds(p->m_allottedScreenArea.GetPosition(), glm::vec2(0.f, 0.f));
			bool boundsSeeded = false;
			size_t cursor = 0;

			for (size_t i = 0; i <= p->m_ownCommands.size(); ++i)
			{
				while (cursor < p->m_childSlots.size() && p->m_childSlots[cursor].insertBefore == i)
				{
					ChildSlot& slot = p->m_childSlots[cursor];

					// Splice pre-built child buffer — child is guaranteed processed already.
					Vector<CircuitDrawCommand>& childOut = perPainterOut.at(slot.painter);
					pOut.insert(pOut.end(), childOut.begin(), childOut.end());

					const Volt::Rect& childBounds = perPainterBounds.at(slot.painter);
					slot.widget->SetBounds(childBounds);
					slot.widget->SetAllotedScreenArea(slot.allottedScreenArea);

					if (!boundsSeeded)
					{
						bounds = childBounds;
						boundsSeeded = true;
					}
					else
					{
						bounds.MergeRectIntoThis(childBounds);
					}

					++cursor;
				}

				if (i < p->m_ownCommands.size())
				{
					PendingDrawCommand& pending = p->m_ownCommands[i];
					CircuitDrawCommand cmd = pending.cmd;

					if (pending.image)
					{
						cmd.textureIndex = p->m_resourceTable->GetOrAddTextureSlotIndex(pending.image);
						cmd.dimensions = { pending.image->GetWidth(), pending.image->GetHeight() };
					}

					// cmd.bounds is in window-local coords; convert to screen for widget bounds.
					Volt::Rect screenCmdBounds(
						glm::vec2(cmd.bounds.x, cmd.bounds.y) + p->m_windowOrigin,
						glm::vec2(cmd.bounds.z - cmd.bounds.x, cmd.bounds.w - cmd.bounds.y));

					if (!boundsSeeded)
					{
						bounds = screenCmdBounds;
						boundsSeeded = true;
					}
					else
					{
						bounds.MergeRectIntoThis(screenCmdBounds);
					}

					pOut.push_back(std::move(cmd));
				}
			}

			perPainterBounds.emplace(p, bounds);

			if (log)
			{
				VT_LOGC(Info, LogCircuit, "[Consolidate] exit  painter={} emittedCmds={} bounds=({},{} {}x{})",
					static_cast<const void*>(p),
					pOut.size(),
					bounds.GetPosition().x, bounds.GetPosition().y,
					bounds.GetSize().x, bounds.GetSize().y);
			}
		}

		Vector<CircuitDrawCommand>& rootOut = perPainterOut[this];
		out.insert(out.end(), rootOut.begin(), rootOut.end());

		return perPainterBounds[this];
	}

	glm::vec2 CircuitPainter::ToPixelPos(const glm::vec2& localPos) const
	{
		return localPos + m_allottedScreenArea.GetPosition() - m_windowOrigin;
	}

	void CircuitPainter::AddDrawCommand(CircuitDrawCommand&& command, IntRef<Volt::RHI::Image> image)
	{
		command.clipRect = {
			ToPixelPos({0,0}),
			ToPixelPos(m_allottedScreenArea.GetSize())
		};

		m_ownCommands.push_back({ std::move(command), image });
	}

	PainterPool::PainterPool(const glm::vec2& windowOrigin, IntRef<Volt::RHI::ResourceTable> resourceTable)
		: m_windowOrigin(windowOrigin),
		m_resourceTable(resourceTable)
	{
	}

	PainterPool::~PainterPool() = default;

	void PainterPool::Reserve(const Ref<Widget>& root)
	{
		VT_PROFILE_FUNCTION();
		const bool log = s_cvarCircuitLogPaint.GetValue() != 0;

		// Breadth-first walk so m_orderedWidgets ends up parent-before-children.
		// Using index cursor avoids vector front-removal cost.
		m_orderedWidgets.push_back(root);

		for (size_t cursor = 0; cursor < m_orderedWidgets.size(); ++cursor)
		{
			Ref<Widget> widget = m_orderedWidgets[cursor];
			if (!widget)
			{
				if (log)
				{
					VT_LOGC(Info, LogCircuit, "[Reserve] idx={} widget=null (skipped)", cursor);
				}
				continue;
			}

			m_painters.emplace(widget.GetRaw(), std::make_unique<CircuitPainter>(m_windowOrigin, m_resourceTable, this));

			Vector<Ref<Widget>> children = widget->GetChildren();
			if (log)
			{
				VT_LOGC(Info, LogCircuit, "[Reserve] idx={} widget={} children={}",
					cursor, static_cast<const void*>(widget.GetRaw()), children.size());
			}

			for (Ref<Widget> child : children)
			{
				if (child)
				{
					m_orderedWidgets.push_back(child);
				}
			}
		}

		if (log)
		{
			VT_LOGC(Info, LogCircuit, "[Reserve] total reserved widgets: {}", m_orderedWidgets.size());
		}
	}

	bool PainterPool::Contains(const Ref<Widget>& widget) const
	{
		return m_painters.find(widget.GetRaw()) != m_painters.end();
	}

	CircuitPainter& PainterPool::GetFor(const Ref<Widget>& widget)
	{
		auto it = m_painters.find(widget.GetRaw());
		VT_ENSURE(it != m_painters.end());
		return *it->second;
	}
}
