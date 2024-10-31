#pragma once
#include "Circuit/Widgets/Widget.h"
#include <CoreUtilities/Containers/Vector.h>

namespace Circuit
{
	class CIRCUIT_API CompoundWidget : public Widget
	{
	public:
		virtual void OnPaint(CircuitPainter& painter) override;
		const Vector<std::shared_ptr<Widget>>& GetChildren();
	protected:
		void AddChildWidget(std::shared_ptr<Widget> childWidget);
	private:
		Vector<std::shared_ptr<Widget>> m_childWidgets;
	};
}
