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

	void LayoutWidget::OnPaint(CircuitPainter& painter)
	{
		float orientationAllotted = -1;
		switch (m_orientation)
		{
			case Circuit::LayoutOrientation::Horizontal:
				orientationAllotted = painter.GetAllottedSize().x;
				break;
			case Circuit::LayoutOrientation::Vertical:
				orientationAllotted = painter.GetAllottedSize().y;
				break;
		}

		float determinedSize = 0;
		int numUndeterminedSlices = 0;

		GlobalMemoryStackMark memMark;
		GlobalMemoryStackVector<float> widgetAllotedSizes(m_slices.size());

		for (size_t i = 0; i < m_slices.size(); i++)
		{
			Slice& slice = m_slices[i];

			// widget size x/y can be -1 here, it means give the widget as much space as possible
			glm::vec2 widgetSize = { -1,-1 };
			if (!slice.widget.IsExpired())
			{
				widgetSize = slice.widget.Lock()->GetDesiredSize();
			}
			else if (!slice.isFlexible)
			{
				continue;
			}

			if (slice.isFlexible)
			{
				float orientationSize = -1;
				switch (m_orientation)
				{
					case Circuit::LayoutOrientation::Horizontal:
						orientationSize = widgetSize.x;
						break;
					case Circuit::LayoutOrientation::Vertical:
						orientationSize = widgetSize.y;
						break;
				}

				if (determinedSize + orientationSize > orientationAllotted)
				{
					const float unitsOverAllotted = (determinedSize + orientationSize) - orientationAllotted;
					orientationSize -= unitsOverAllotted;
				}

				widgetAllotedSizes[i] = orientationSize;
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
					sizeLeftForUndeterminedFlexibleSlices = painter.GetAllottedSize().x - determinedSize;
					break;
				case Circuit::LayoutOrientation::Vertical:
					sizeLeftForUndeterminedFlexibleSlices = painter.GetAllottedSize().y - determinedSize;
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
			if (!m_slices[i].widget.IsExpired())
			{
				Ref<Widget> widget = m_slices[i].widget.Lock();

				const float margin = m_slices[i].margin;

				switch (m_orientation)
				{
					case Circuit::LayoutOrientation::Horizontal:
						painter.AddWidget(widget, currentSliceOffset + margin, margin, widgetAllotedSizes[i] - margin * 2, painter.GetAllottedSize().y - margin * 2);
						break;
					case Circuit::LayoutOrientation::Vertical:
						painter.AddWidget(widget, margin, currentSliceOffset + margin, painter.GetAllottedSize().x - margin * 2, widgetAllotedSizes[i] - margin * 2);
						break;
				}
			}

			currentSliceOffset += widgetAllotedSizes[i];
		}
	}

	void LayoutWidget::AddSpring()
	{
		VT_PROFILE_FUNCTION();
		Slice& newSlice = m_slices.emplace_back();
		newSlice.isFlexible = true;
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
