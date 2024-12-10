#pragma once
#include "Circuit/Widgets/BuildingSyntaxUtility.h"
#include "Circuit/Config.h"

#include <CoreUtilities/Math/2DShapes/Rect.h>

namespace Circuit
{
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
		void SetX(float x) { m_LocalXPosition = x; }
		float GetX() const { return m_LocalXPosition; }

		void SetY(float y) { m_LocalYPosition = y; }
		float GetY() const { return m_LocalYPosition; }


		void BuildBaseArgs(const CircuitBaseArgs& baseArgs);

		virtual void OnPaint(CircuitPainter& painter);
		//AllotedSize on specific axis will be -1 if it depends on the child widget for that size
		//Returns the size of the widget if placed in the given alloted area
		virtual glm::vec2 OnLayout(const glm::vec2& allotedSize) { return glm::vec2(0, 0); };

		Volt::Rect GetBounds() { return m_bounds; }

		void RequestRebuild();

		virtual bool HasChildren() const { return false; }
		virtual const Vector<std::shared_ptr<Widget>>* GetChildren() const { return nullptr;}
		bool IsRenderPrimitive() const;
		RenderPrimitiveType GetRenderPrimitiveType() const;

		Weak<Widget> GetParent() { return m_parentWidget; }
		virtual bool IsHittestInvisible() const { return false; };

		virtual void OnBeginHover() {}
		virtual void OnEndHover() {}
		virtual void OnPressed() {}
		virtual void OnReleased() {}


	protected:
		template<class WidgetType>
		inline Ref<WidgetType>& AddChildWidget(Ref<WidgetType> Widget);

		bool m_IsRenderPrimitive = false;
		RenderPrimitiveType m_RenderPrimitiveType;


		Weak<Widget> m_parentWidget;
	private:
		Volt::Rect m_bounds = Volt::Rect(0,0,0,0);

		float m_LocalXPosition = 0;
		float m_LocalYPosition = 0;
		bool m_NeedsRebuild;


	};
}
