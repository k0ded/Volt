#include "circuitpch.h"
#include "CompoundWidget.h"
#include "CircuitPainter.h"

namespace Circuit
{
	void Circuit::CompoundWidget::OnPaint(CircuitPainter& painter)
	{
		for (std::shared_ptr<Widget> childWidget : m_childWidgets)
		{
			childWidget->OnPaint(painter);
		}
	}

	const Vector<std::shared_ptr<Widget>>& Circuit::CompoundWidget::GetChildren()
	{
		return m_childWidgets;
	}

	void Circuit::CompoundWidget::AddChildWidget(std::shared_ptr<Widget> childWidget)
	{
		m_childWidgets.push_back(childWidget);
		childWidget->
		CalculateBounds();
	}

}
