#pragma once
#include "Circuit/Widgets/BuildingSyntaxUtility.h"
#include "Circuit/Config.h"

#include <CoreUtilities/Math/2DShapes/Rect.h>
#include <CoreUtilities/Pointers/Weak.h>

#include <glm/fwd.hpp>

namespace Circuit
{
	struct WidgetInteractionData;

	class CircuitPainter;
	enum class RenderPrimitiveType
	{
		Rectangle,
		Circle
	};
	class CIRCUIT_API Widget
	{
	public:
		Widget() {};
		virtual ~Widget() {};

	public:
		void BuildBaseArgs(const CircuitBaseArgs& baseArgs);

		virtual void OnPaint(CircuitPainter& painter);

		virtual glm::vec2 GetDesiredSize() = 0;

		const Volt::Rect& GetBounds() const { return m_bounds; }
		void SetBounds(const Volt::Rect& newBounds) { m_bounds = newBounds; }

		const Volt::Rect& GetAllotedScreenArea() const { return m_allotedScreenArea; }
		void SetAllotedScreenArea(const Volt::Rect& newAllotedScreenArea) { m_allotedScreenArea = newAllotedScreenArea; }

		void RequestRebuild();

		virtual bool HasChildren() const { return false; }
		virtual Vector<Ref<Widget>> GetChildren();
		bool IsRenderPrimitive() const;
		RenderPrimitiveType GetRenderPrimitiveType() const;

		Weak<Widget> GetParent() { return m_parentWidget; }
		virtual bool IsHittestInvisible() const { return false; };

		virtual void OnBeginHover(const WidgetInteractionData& interactionData) {}
		virtual void OnEndHover(const WidgetInteractionData& interactionData) {}
		virtual void OnPressed(const WidgetInteractionData& interactionData) {}
		virtual void OnReleased(const WidgetInteractionData& interactionData) {}
		virtual void OnBeginDrag(const WidgetInteractionData& interactionData) {}
		virtual void OnDrag(const WidgetInteractionData& interactionData){}
		virtual void OnEndDrag(const WidgetInteractionData& interactionData){}


		void SetLocalPosition(const glm::vec2& localPos);
	protected:


		bool m_IsRenderPrimitive = false;
		RenderPrimitiveType m_RenderPrimitiveType;


		Weak<Widget> m_parentWidget;
	private:
		glm::vec2 m_localPosition;
		Volt::Rect m_bounds = Volt::Rect(0, 0, -1, -1);
		Volt::Rect m_allotedScreenArea = Volt::Rect(0, 0, -1, -1);
		bool m_NeedsRebuild;
	};
}
