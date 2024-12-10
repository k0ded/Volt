#pragma once
#include "Circuit/Widgets/Widget.h"
#include <CoreUtilities/Containers/Vector.h>

namespace Circuit
{
	class CIRCUIT_API CompoundWidget : public Widget
	{
	public:
		virtual glm::vec2 OnLayout(const glm::vec2& allotedSize) override;
		virtual void OnPaint(CircuitPainter& painter) override;

		const Vector<std::shared_ptr<Widget>>* GetChildren() const override { return &m_childWidgets; }
		bool HasChildren() const { return !m_childWidgets.empty(); }

	protected:
		void AddChildWidget(std::shared_ptr<Widget> childWidget);
	private:
		Vector<std::shared_ptr<Widget>> m_childWidgets;
	};
}
