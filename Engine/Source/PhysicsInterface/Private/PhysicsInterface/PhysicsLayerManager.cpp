#include "pipch.h"

#include "PhysicsInterface/PhysicsLayerManager.h"

Volt::PhysicsLayerManager g_physicsLayerManager;

namespace Volt
{
	bool PhysicsLayerManager::AreLayersColliding(PhysicsLayerID layerA, PhysicsLayerID layerB)
	{
		VT_ENSURE(m_layers.size() > static_cast<size_t>(layerA) && m_layers.size() > static_cast<size_t>(layerB));
		return m_layers.at(layerA).collidesWithBitMask & m_layers.at(layerB).bit || m_layers.at(layerB).collidesWithBitMask & m_layers.at(layerA).bit;
	}

	const PhysicsLayer& PhysicsLayerManager::GetLayer(PhysicsLayerID layerId) const
	{
		VT_ENSURE(m_layers.size() > static_cast<size_t>(layerId));
		return m_layers.at(layerId);
	}
}
