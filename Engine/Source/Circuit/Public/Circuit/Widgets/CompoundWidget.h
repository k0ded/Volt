#pragma once
#include "Circuit/Widgets/Widget.h"
#include <CoreUtilities/Containers/Vector.h>

namespace Circuit
{
	class CIRCUIT_API CompoundWidget : public Widget
	{
	public:
		virtual glm::vec2 GetDesiredSize() override;

		virtual void OnPaint(CircuitPainter& painter) override;

		Vector<Ref<Widget>> GetChildren() override { return m_childWidgets; }
		bool HasChildren() const override { return !m_childWidgets.empty(); }

	protected:
		void AddChildWidget(Ref<Widget> childWidget);
		void RemoveChildWidget(Ref<Widget> childWidget);
		void ClearChildWidgets();
	private:
		Vector<Ref<Widget>> m_childWidgets;
	};
}
