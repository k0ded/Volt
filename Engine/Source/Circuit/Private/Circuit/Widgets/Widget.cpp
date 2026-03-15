#include "circuitpch.h"
#include "Widgets/Widget.h"

#include "CircuitPainter.h"

#include <CoreUtilities/Containers/Vector.h>

#include <WindowModule/WindowManager.h>
#include <WindowModule/Window.h>

namespace Circuit
{

	void Widget::BuildBaseArgs(const CircuitBaseArgs& baseArgs)
	{
		//m_LocalXPosition = baseArgs._X;
		//m_LocalYPosition = baseArgs._Y;
	}

	void Widget::OnPaint(CircuitPainter& painter)
	{
	}

	void Widget::RequestRebuild()
	{
		m_NeedsRebuild = true;
	}

	Vector<std::shared_ptr<Widget>> Widget::GetChildren()
	{
		return Vector<std::shared_ptr<Widget>>();
	}

	bool Widget::IsRenderPrimitive() const
	{
		return m_IsRenderPrimitive;
	}

	RenderPrimitiveType Widget::GetRenderPrimitiveType() const
	{
		return m_RenderPrimitiveType;
	}

	void Widget::SetLocalPosition(const glm::vec2& localPos)
	{
		m_localPosition = localPos;
	}

}
