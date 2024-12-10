#include "circuitpch.h"
#include "LayoutWidget.h"
#include "Circuit/CircuitPainter.h"

namespace Circuit
{
	void LayoutWidget::Build(const Arguments& args)
	{
		m_orientation = args._Orientation;
	}
	glm::vec2 LayoutWidget::OnLayout(const glm::vec2& allotedSize)
	{
		glm::vec2 returnSize = glm::zero<glm::vec2>();
		switch (m_orientation)
		{
			case Circuit::LayoutOrientation::Horizontal:
				returnSize.y = 0;//highest slice
				returnSize.x = 0;//all slices widths together
				break;
			case Circuit::LayoutOrientation::Vertical:
				returnSize.x = 0;//widest slice
				returnSize.y = 0;//all slices heights together
				break;
		}

		for (Slice& slice : m_slices)
		{
			if (slice.widget.IsExpired())
			{
				continue;
			}


			//determine what size we are giving the widget in the slice
			glm::vec2 sliceSize = { 0,0 };
			float orientationSize = 0;
			if (slice.isFlexible)
			{
				orientationSize = -1;
			}
			else
			{
				orientationSize = slice.size;
			}

			switch (m_orientation)
			{
				case Circuit::LayoutOrientation::Horizontal:
					sliceSize.x = orientationSize;
					sliceSize.y = allotedSize.y;
					break;
				case Circuit::LayoutOrientation::Vertical:
					sliceSize.y = orientationSize;
					sliceSize.x = allotedSize.x;
					break;
			}
			//

			const glm::vec2 widgetSize = slice.widget->OnLayout(sliceSize);


			if (sliceSize.x == -1)
			{
				sliceSize.x = widgetSize.x;
			}

			if (sliceSize.y == -1)
			{
				sliceSize.y = widgetSize.y;
			}


			switch (m_orientation)
			{
				case LayoutOrientation::Horizontal:
					returnSize.x += sliceSize.x; // all slices widths summed
					returnSize.y = glm::max(sliceSize.y, returnSize.y); // the tallest slice

					if (slice.isFlexible)
					{
						slice.size = sliceSize.x;
					}
					break;

				case LayoutOrientation::Vertical:
					returnSize.y += sliceSize.y; // all slices heights summed
					returnSize.x = glm::max(sliceSize.x, returnSize.x); // the widest slice

					if (slice.isFlexible)
					{
						slice.size = sliceSize.y;
					}
					break;
			}
		}

		return returnSize;
	}
	void LayoutWidget::OnPaint(CircuitPainter& painter)
	{
		float offset = 0;
		for (Slice& slice : m_slices)
		{
			if (slice.widget.IsExpired())
			{
				continue;
			}

			switch (m_orientation)
			{
				case Circuit::LayoutOrientation::Horizontal:
					painter.AddWidget(slice.widget, offset, 0, slice.size, painter.GetAllotedArea().GetSize().y);
					break;
				case Circuit::LayoutOrientation::Vertical:
					painter.AddWidget(slice.widget, 0, offset, painter.GetAllotedArea().GetSize().x, slice.size);
					break;
			}

			offset += slice.size;
		}
	}
	void LayoutWidget::AddFixedSlice(Ref<Widget> contentWidget, float size)
	{
		VT_PROFILE_FUNCTION();
		Slice& newSlice = m_slices.emplace_back();
		newSlice.widget = contentWidget;
		newSlice.size = static_cast<float>(size);
		newSlice.isFlexible = false;

		AddChildWidget(contentWidget);
	}

	void LayoutWidget::AddFlexibleSlice(Ref<Widget> contentWidget)
	{
		Slice& newSlice = m_slices.emplace_back();
		newSlice.widget = contentWidget;
		newSlice.isFlexible = true;

		AddChildWidget(contentWidget);
	}


}
