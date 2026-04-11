#pragma once
#include "Circuit/CircuitDrawCommand.h"

#include <Volt-Assets/FontAsset.h>

#include <RHIModule/Descriptors/ResourceTable.h>
#include <RHIModule/Images/Image.h>

#include <AssetSystem/AssetReference.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Math/2DShapes/Rect.h>

namespace Circuit
{
	class Widget;
	class CIRCUIT_API CircuitPainter
	{
	public:
		CircuitPainter(const Volt::Rect& allotedScreenArea, IntRef<Volt::RHI::ResourceTable> resourceTable)
			: m_allottedScreenArea(allotedScreenArea), 
			m_basePainter(this),
			m_resourceTable(resourceTable)
		{};

		~CircuitPainter() = default;

		glm::vec2 GetAllottedSize() const;

		VT_INLINE void AddWidget(Ref<Widget> widget, float x, float y, float width, float height) { AddWidget(widget,Volt::Rect(x, y, width, height)); }
		VT_INLINE void AddWidget(Ref<Widget> widget, const glm::vec2& position, const glm::vec2& size){AddWidget(widget, Volt::Rect(position, size));}
		void AddWidget(Ref<Widget> widget, const Volt::Rect& allotedArea);

		void AddRect(float x, float y, float width, float height, CircuitColor color, float rotation = 0, float scale = 1);
		void AddRectOutline(float x, float y, float width, float height, CircuitColor color, float lineThickness, float rotation = 0, float scale = 1);

		void AddCircle(float x, float y, float radius, CircuitColor color, float scale = 1);
		void AddCircleSegment(float x, float y, float innerRadius, float outerRadius, float angleDegrees, CircuitColor color, float scale = 1);
		void AddLine(float x0, float y0, float x1, float y1, float radius, CircuitColor color);
		void AddText(float x, float y, const String& text, AssetReference<Volt::FontAsset> font, float maxWidth, CircuitColor color, float scale = 1.f);
		void AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float scale = 1.f);
		void AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float uv0x, float uv0y, float uv1x, float uv1y, float scale = 1.f);

		ArrayView<CircuitDrawCommand> GetCommands();

	private:
		CircuitPainter(CircuitPainter* basePainter, CircuitPainter* parentPainter, const Volt::Rect& allotedScreenArea, IntRef<Volt::RHI::ResourceTable> resourceTable)
			: m_allottedScreenArea(allotedScreenArea),
			m_basePainter(basePainter),
			m_parentPainter(parentPainter),
			m_resourceTable(resourceTable)
		{};

		VT_INLINE CircuitPainter CreateSubPainter(const Volt::Rect& allotedScreenArea) { return CircuitPainter(m_basePainter ? m_basePainter : this, this, allotedScreenArea, m_resourceTable); }

		glm::vec2 ToPixelPos(const glm::vec2& localPos);
		void AddDrawCommand(CircuitDrawCommand&& command);

		Vector<CircuitDrawCommand> m_drawCommands;

		Volt::Rect m_allottedScreenArea;

		CircuitPainter* m_basePainter = nullptr;
		CircuitPainter* m_parentPainter = nullptr;
		IntRef<Volt::RHI::ResourceTable> m_resourceTable;
	};
}
