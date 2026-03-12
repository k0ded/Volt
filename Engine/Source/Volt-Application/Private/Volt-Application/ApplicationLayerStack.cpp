#include "vtapppch.h"
#include "Volt-Application/ApplicationLayerStack.h"
#include "Volt-Application/ApplicationLayer.h"

namespace Volt
{
	ApplicationLayerStack::ApplicationLayerStack()
	{}

	ApplicationLayerStack::~ApplicationLayerStack()
	{
		Clear();
	}

	void ApplicationLayerStack::PushLayer(ApplicationLayer* layer)
	{
		m_layers.emplace(m_layers.begin() + m_lastInsertIndex, layer);
		m_lastInsertIndex++;
		layer->OnAttach();
	}

	void ApplicationLayerStack::PushOverlay(ApplicationLayer* overlay)
	{
		m_layers.emplace_back(overlay);
		overlay->OnAttach();
	}

	void ApplicationLayerStack::PopLayer(ApplicationLayer* layer)
	{
		auto it = std::find(m_layers.begin(), m_layers.begin() + m_lastInsertIndex, layer);
		if (it != m_layers.begin() + m_lastInsertIndex)
		{
			layer->OnDetach();
			m_layers.erase(it);
			m_lastInsertIndex--;
			delete layer;
		}
	}

	void ApplicationLayerStack::PopOverlay(ApplicationLayer* overlay)
	{
		auto it = std::find(m_layers.begin() + m_lastInsertIndex, m_layers.end(), overlay);
		if (it != m_layers.end())
		{
			overlay->OnDetach();
			m_layers.erase(it);
			delete overlay;
		}
	}

	void ApplicationLayerStack::PopLast()
	{
		ApplicationLayer* lastLayer = m_layers.back();
		PopLayer(lastLayer);
	}

	ApplicationLayer* ApplicationLayerStack::GetLastLayer()
	{
		if (m_layers.empty())
		{
			return nullptr;
		}
		return m_layers.back();
	}

	void ApplicationLayerStack::Clear()
	{
		for (ApplicationLayer* layer : m_layers)
		{
			layer->OnDetach();
			delete layer;
		}

		m_lastInsertIndex = 0;
		m_layers.clear();
	}
}
