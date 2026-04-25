#pragma once
#include "Volt-Application/Config.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt
{
	class ApplicationLayer;
	class VTAPP_API ApplicationLayerStack
	{
	public:
		ApplicationLayerStack();
		~ApplicationLayerStack();

		void PushLayer(ApplicationLayer* layer);
		void PushOverlay(ApplicationLayer* layer);
		void PopLayer(ApplicationLayer* layer);
		void PopOverlay(ApplicationLayer* layer);

		void PopLast();
		ApplicationLayer* GetLastLayer();

		void Clear();

		ArrayView<ApplicationLayer*> GetLayerStack() { return m_layers; }

		Vector<ApplicationLayer*>::iterator begin() { return m_layers.begin(); }
		Vector<ApplicationLayer*>::iterator end() { return m_layers.end(); }

	private:
		Vector<ApplicationLayer*> m_layers;
		uint32_t m_lastInsertIndex = 0;
	};
}
