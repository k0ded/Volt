#include "circuitpch.h"
#include "LayoutWidget.h"
#include "Circuit/CircuitPainter.h"

#include <CoreUtilities/Containers/VectorVariants.h>

namespace Circuit
{
	void LayoutWidget::Build(const Arguments& args)
	{
		m_orientation = args._Orientation;
	}
	void LayoutWidget::OnLayout(const glm::vec2& allotedSize)
	{
		//glm::vec2 returnSize = glm::zero<glm::vec2>();
		//switch (m_orientation)
		//{
		//	case Circuit::LayoutOrientation::Horizontal:
		//		returnSize.y = 0;//highest slice
		//		returnSize.x = 0;//all slices widths together
		//		break;
		//	case Circuit::LayoutOrientation::Vertical:
		//		returnSize.x = 0;//widest slice
		//		returnSize.y = 0;//all slices heights together
		//		break;
		//}

		//float determinedSize = 0;
		//int numUndeterminedSlices = 0;
		//Vector<float> widgetAllotedSizes(m_slices.size());
		//for (size_t i = 0; i < m_slices.size(); i++)
		//{
		//	Slice& slice = m_slices[i];
		//	if (slice.widget.IsExpired())
		//	{
		//		continue;
		//	}

		//	// widget size x/y can be -1 here, it means give the widget as much space as possible
		//	const glm::vec2 widgetSize = slice.widget->GetDesiredSize();

		//	if (slice.isFlexible)
		//	{
		//		switch (m_orientation)
		//		{
		//			case Circuit::LayoutOrientation::Horizontal:
		//				widgetAllotedSizes[i] = widgetSize.x;
		//				break;
		//			case Circuit::LayoutOrientation::Vertical:
		//				widgetAllotedSizes[i] = widgetSize.y;
		//				break;
		//		}
		//	}
		//	else
		//	{
		//		widgetAllotedSizes[i] = slice.size;
		//	}

		//	if (widgetAllotedSizes[i] == -1)
		//	{
		//		numUndeterminedSlices++;
		//	}
		//	else
		//	{
		//		determinedSize = widgetAllotedSizes[i];
		//	}

		//}

		//if (numUndeterminedSlices > 0)
		//{
		//	float sizeLeftForUndeterminedFlexibleSlices = 0;
		//	switch (m_orientation)
		//	{
		//		case Circuit::LayoutOrientation::Horizontal:
		//			sizeLeftForUndeterminedFlexibleSlices = allotedSize.x - determinedSize;
		//			break;
		//		case Circuit::LayoutOrientation::Vertical:
		//			sizeLeftForUndeterminedFlexibleSlices = allotedSize.y - determinedSize;
		//			break;
		//	}
		//	VT_ASSERT(sizeLeftForUndeterminedFlexibleSlices >= 0);

		//	const float undeterminedSliceAllotedSize = sizeLeftForUndeterminedFlexibleSlices / numUndeterminedSlices;
		//	for (size_t i = 0; i < widgetAllotedSizes.size(); i++)
		//	{
		//		if (widgetAllotedSizes[i] == -1)
		//		{
		//			widgetAllotedSizes[i] = undeterminedSliceAllotedSize;
		//		}
		//	}

		//}


		//float currentSliceOffset = 0;
		//for (size_t i = 0; i < m_slices.size(); i++)
		//{
		//	switch (m_orientation)
		//	{
		//		case Circuit::LayoutOrientation::Horizontal:
		//			SetChildOffset(m_slices[i].widget, { currentSliceOffset, 0 }, widgetAllotedSizes[i]);
		//			break;
		//		case Circuit::LayoutOrientation::Vertical:
		//			SetChildOffset(m_slices[i].widget, { 0, currentSliceOffset }, widgetAllotedSizes[i]);
		//			break;
		//	}

		//	currentSliceOffset += widgetAllotedSizes[i];
		//}
	}
	void LayoutWidget::OnPaint(CircuitPainter& painter)
	{
		float determinedSize = 0;
		int numUndeterminedSlices = 0;
		Vector<float> widgetAllotedSizes(m_slices.size());
		for (size_t i = 0; i < m_slices.size(); i++)
		{
			Slice& slice = m_slices[i];
			if (slice.widget.IsExpired())
			{
				continue;
			}

			// widget size x/y can be -1 here, it means give the widget as much space as possible
			const glm::vec2 widgetSize = slice.widget->GetDesiredSize();

			if (slice.isFlexible)
			{
				switch (m_orientation)
				{
					case Circuit::LayoutOrientation::Horizontal:
						widgetAllotedSizes[i] = widgetSize.x;
						break;
					case Circuit::LayoutOrientation::Vertical:
						widgetAllotedSizes[i] = widgetSize.y;
						break;
				}
			}
			else
			{
				widgetAllotedSizes[i] = slice.size;
			}

			if (widgetAllotedSizes[i] == -1)
			{
				numUndeterminedSlices++;
			}
			else
			{
				determinedSize += widgetAllotedSizes[i];
			}

		}

		if (numUndeterminedSlices > 0)
		{
			float sizeLeftForUndeterminedFlexibleSlices = 0;
			switch (m_orientation)
			{
				case Circuit::LayoutOrientation::Horizontal:
					sizeLeftForUndeterminedFlexibleSlices = painter.GetAllotedSize().x - determinedSize;
					break;
				case Circuit::LayoutOrientation::Vertical:
					sizeLeftForUndeterminedFlexibleSlices = painter.GetAllotedSize().y - determinedSize;
					break;
			}
			VT_ASSERT(sizeLeftForUndeterminedFlexibleSlices >= 0);

			const float undeterminedSliceAllotedSize = sizeLeftForUndeterminedFlexibleSlices / numUndeterminedSlices;
			for (size_t i = 0; i < widgetAllotedSizes.size(); i++)
			{
				if (widgetAllotedSizes[i] == -1)
				{
					widgetAllotedSizes[i] = undeterminedSliceAllotedSize;
				}
			}
		}

		float currentSliceOffset = 0;
		for (size_t i = 0; i < m_slices.size(); i++)
		{
			const float margin = m_slices[i].margin;

			switch (m_orientation)
			{
				case Circuit::LayoutOrientation::Horizontal:
					painter.AddWidget(m_slices[i].widget, currentSliceOffset + margin, margin, widgetAllotedSizes[i]- margin*2, painter.GetAllotedSize().y- margin*2);
					break;
				case Circuit::LayoutOrientation::Vertical:
					painter.AddWidget(m_slices[i].widget, margin, currentSliceOffset + margin, painter.GetAllotedSize().x - margin * 2, widgetAllotedSizes[i] - margin * 2);
					break;
			}

			currentSliceOffset += widgetAllotedSizes[i];
		}
	}

	void LayoutWidget::AddFixedSlice(Ref<Widget> contentWidget, float size, float margin)
	{
		VT_PROFILE_FUNCTION();
		Slice& newSlice = m_slices.emplace_back();
		newSlice.widget = contentWidget;
		newSlice.size = size;
		newSlice.margin = margin;
		newSlice.isFlexible = false;

		AddChildWidget(contentWidget);
	}

	void LayoutWidget::AddFlexibleSlice(Ref<Widget> contentWidget, float margin)
	{
		Slice& newSlice = m_slices.emplace_back();
		newSlice.margin = margin;
		newSlice.widget = contentWidget;
		newSlice.isFlexible = true;

		AddChildWidget(contentWidget);
	}


}
