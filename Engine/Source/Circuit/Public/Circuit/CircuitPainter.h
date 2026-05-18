#pragma once
#include "Circuit/CircuitDrawCommand.h"

#include <Volt-Assets/FontAsset.h>

#include <RHIModule/Descriptors/ResourceTable.h>
#include <RHIModule/Images/Image.h>

#include <AssetSystem/AssetReference.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Math/2DShapes/Rect.h>

#include <memory>

namespace Circuit
{
	class Widget;
	class PainterPool;

	enum class RectCorners : uint8_t
	{
		None = 0,
		TopLeft = 1 << 0,
		TopRight = 1 << 1,
		BottomLeft = 1 << 2,
		BottomRight = 1 << 3,

		Top = TopLeft | TopRight,
		Bottom = BottomLeft | BottomRight,
		Left = TopLeft | BottomLeft,
		Right = TopRight | TopLeft,

		All = TopLeft | TopRight | BottomLeft | BottomRight
	};

	class CIRCUIT_API CircuitPainter
	{
	public:
		CircuitPainter(const glm::vec2& windowOrigin, IntRef<Volt::RHI::ResourceTable> resourceTable, PainterPool* pool = nullptr);

		~CircuitPainter();
		CircuitPainter(const CircuitPainter&) = delete;
		CircuitPainter& operator=(const CircuitPainter&) = delete;
		CircuitPainter(CircuitPainter&&) noexcept;
		CircuitPainter& operator=(CircuitPainter&&) noexcept;

		void SetAllottedScreenArea(const Volt::Rect& allottedScreenArea);
		const Volt::Rect& GetAllottedScreenArea() const { return m_allottedScreenArea; }
		glm::vec2 GetAllottedSize() const;

		VT_INLINE void AddWidget(Ref<Widget> widget, float x, float y, float width, float height) { AddWidget(widget, Volt::Rect(x, y, width, height)); }
		VT_INLINE void AddWidget(Ref<Widget> widget, const glm::vec2& position, const glm::vec2& size) { AddWidget(widget, Volt::Rect(position, size)); }
		void AddWidget(Ref<Widget> widget, const Volt::Rect& allottedLocalArea);

		void AddRect(float x, float y, float width, float height, CircuitColor color, float rounding = 0,RectCorners roundingCorners = RectCorners::All, float rotation = 0, float scale = 1);
		void AddRectOutline(float x, float y, float width, float height, CircuitColor color, float lineThickness, float rounding = 0, RectCorners roundingCorners = RectCorners::All, float rotation = 0, float scale = 1);

		void AddCircle(float x, float y, float radius, CircuitColor color, float scale = 1);
		void AddCircleSegment(float x, float y, float innerRadius, float outerRadius, float angleDegrees, CircuitColor color, float scale = 1);
		void AddLine(float x0, float y0, float x1, float y1, float radius, CircuitColor color);
		void AddText(float x, float y, const String& text, AssetReference<Volt::FontAsset> font, float maxWidth, CircuitColor color, float scale = 1.f);
		void AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float scale = 1.f);
		void AddImage(float x, float y, float width, float height, IntRef<Volt::RHI::Image> image, float uv0x, float uv0y, float uv1x, float uv1y, float scale = 1.f);

		// Walk subtree, splice draw commands into out in paint order, resolve image slot indices,
		// set bounds + alloted screen area on every child widget. Returns screen-space bounds of subtree.
		Volt::Rect Consolidate(Vector<CircuitDrawCommand>& out);

	private:
		struct PendingDrawCommand
		{
			CircuitDrawCommand cmd;
			IntRef<Volt::RHI::Image> image;
		};

		struct ChildSlot
		{
			size_t insertBefore = 0;
			Ref<Widget> widget;
			Volt::Rect allottedScreenArea;
			CircuitPainter* painter = nullptr;
		};

		glm::vec2 ToPixelPos(const glm::vec2& localPos) const;
		void AddDrawCommand(CircuitDrawCommand&& command, IntRef<Volt::RHI::Image> image = nullptr);

		Vector<PendingDrawCommand> m_ownCommands;
		Vector<ChildSlot> m_childSlots;

		Volt::Rect m_allottedScreenArea;
		glm::vec2 m_windowOrigin;
		IntRef<Volt::RHI::ResourceTable> m_resourceTable;
		PainterPool* m_pool = nullptr;
	};

	// Owns a CircuitPainter per widget for the duration of a single window paint pass.
	// Reserve(root) walks the widget tree and pre-allocates a painter for every widget,
	// recording widgets in topological order (parent before children) so the caller can
	// drive OnPaint manually.
	class CIRCUIT_API PainterPool
	{
	public:
		PainterPool(const glm::vec2& windowOrigin, IntRef<Volt::RHI::ResourceTable> resourceTable);
		~PainterPool();

		PainterPool(const PainterPool&) = delete;
		PainterPool& operator=(const PainterPool&) = delete;

		void Reserve(const Ref<Widget>& root);

		bool Contains(const Ref<Widget>& widget) const;
		CircuitPainter& GetFor(const Ref<Widget>& widget);

		const Vector<Ref<Widget>>& GetReservedWidgets() const { return m_orderedWidgets; }

	private:
		glm::vec2 m_windowOrigin;
		IntRef<Volt::RHI::ResourceTable> m_resourceTable;
		Map<Widget*, std::unique_ptr<CircuitPainter>> m_painters;
		Vector<Ref<Widget>> m_orderedWidgets;
	};
}
