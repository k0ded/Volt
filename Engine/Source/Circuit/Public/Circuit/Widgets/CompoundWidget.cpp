#include "circuitpch.h"
#include "CompoundWidget.h"
#include "CircuitPainter.h"

namespace Circuit
{
	glm::vec2 CompoundWidget::GetDesiredSize()
	{
		glm::vec2 size = 0;

		for (std::shared_ptr<Widget> child : m_childWidgets)
		{
			glm::vec2 childDesiredSize = child->GetDesiredSize();
			size.x = glm::max(childDesiredSize.x, size.x);
			size.y = glm::max(childDesiredSize.y, size.y);
		}
		return size;
	}

	void Circuit::CompoundWidget::OnPaint(CircuitPainter& painter)
	{
		for (std::shared_ptr<Widget> childWidget : m_childWidgets)
		{
			painter.AddWidget(childWidget, { 0,0 }, painter.GetAllotedSize());
		}
	}

	void CompoundWidget::AddChildWidget(Ref<Widget> childWidget)
	{
		m_childWidgets.push_back(childWidget);
	}
}
