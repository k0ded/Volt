#pragma once
#include "Circuit/Widgets/Widget.h"
#include "Circuit/Widgets/CompoundWidget.h"

#include <CoreUtilities/Containers/Vector.h>

namespace Circuit
{
	enum class LayoutOrientation
	{
		Horizontal,
		Vertical,
	};
	class CIRCUIT_API LayoutWidget : public Circuit::CompoundWidget
	{
	public:
		CIRCUIT_BEGIN_ARGS(LayoutWidget)
			:_Orientation(LayoutOrientation::Vertical)
		{
		};

		CIRCUIT_ARGUMENT(LayoutOrientation, Orientation);
		CIRCUIT_END_ARGS();


		void Build(const Arguments& args);

		virtual void OnLayout(const glm::vec2& allotedSize) override;
		//take as much space as possible for now
		virtual glm::vec2 GetDesiredSize() override { return { -1,-1 }; }


		virtual void OnPaint(CircuitPainter& painter) override;

		void AddFixedSlice(Ref<Widget> widget, float size, float margin = 0);

		void AddFlexibleSlice(Ref<Widget> contentWidget, float margin = 0);

		virtual bool IsHittestInvisible() const override { return true; };

	private:

		struct Slice
		{
			Weak<Widget> widget;
			float size = -1;
			float margin = 0; 
			bool isFlexible;
		};
		Vector<Slice> m_slices;

		LayoutOrientation m_orientation;
	};
}
