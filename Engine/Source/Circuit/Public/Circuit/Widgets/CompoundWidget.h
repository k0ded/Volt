#pragma once
#include "Circuit/Widgets/Widget.h"
#include <CoreUtilities/Containers/Vector.h>

namespace Circuit
{
	class CIRCUIT_API CompoundWidget : public Widget
	{
	public:
		virtual void OnLayout(const glm::vec2& allotedSize) override;
		virtual glm::vec2 GetDesiredSize() override;

		virtual void OnPaint(CircuitPainter& painter) override;

		Vector<Ref<Widget>> GetChildren() override { return m_childWidgets; }
		bool HasChildren() const { return !m_childWidgets.empty(); }

	protected:
		void AddChildWidget(Ref<Widget> childWidget);
	private:
		Vector<Ref<Widget>> m_childWidgets;
	};
}
