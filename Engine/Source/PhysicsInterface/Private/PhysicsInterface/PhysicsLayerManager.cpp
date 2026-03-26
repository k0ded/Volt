#include "pipch.h"

#include "PhysicsInterface/PhysicsLayerManager.h"

Volt::PhysicsLayerManager g_physicsLayerManager;

namespace Volt
{
	void PhysicsLayerManager::AddLayer(const String& name)
	{
		auto& newLayer = m_layers.emplace_back();
		newLayer.id = GetNextLayerID();
		newLayer.bit = BIT(newLayer.id);

		for (const auto& otherLayer : m_layers)
		{
			SetLayerCollision(newLayer.id, otherLayer.id, true);
		}
	}

	bool PhysicsLayerManager::AreLayersColliding(PhysicsLayerID layerA, PhysicsLayerID layerB)
	{
		VT_ENSURE(m_layers.size() >= static_cast<size_t>(layerA) && m_layers.size() >= static_cast<size_t>(layerB));
		return m_layers.at(layerA).collidesWithBitMask & m_layers.at(layerB).bit || m_layers.at(layerB).collidesWithBitMask & m_layers.at(layerA).bit;
	}

	const PhysicsLayer& PhysicsLayerManager::GetLayer(PhysicsLayerID layerId) const
	{
		VT_ENSURE(m_layers.size() > static_cast<size_t>(layerId));
		return m_layers.at(layerId);
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(PhysicsLayerID layerId)
	{
		VT_ENSURE(m_layers.size() > static_cast<size_t>(layerId));
		return m_layers.at(layerId);
	}

	uint32_t PhysicsLayerManager::GetNextLayerID() const
	{
		int32_t lastId = -1;

		for (const auto& layer : m_layers)
		{
			if (lastId != -1)
			{
				if (static_cast<int32_t>(layer.id) != lastId + 1)
				{
					return lastId + 1;
				}
			}

			lastId = layer.id;
		}

		return static_cast<uint32_t>(m_layers.size() - 1);
	}

	void PhysicsLayerManager::SetLayerCollision(uint32_t layer, uint32_t otherLayer, bool shouldCollide)
	{
		if (AreLayersColliding(layer, otherLayer) && shouldCollide)
		{
			return;
		}

		auto& layerInfo = GetLayer(layer);
		auto& otherLayerInfo = GetLayer(otherLayer);

		if (shouldCollide)
		{
			layerInfo.collidesWithBitMask |= otherLayerInfo.bit;
			otherLayerInfo.collidesWithBitMask |= layerInfo.bit;
		}
		else
		{
			layerInfo.collidesWithBitMask &= ~otherLayerInfo.bit;
			otherLayerInfo.collidesWithBitMask &= ~layerInfo.bit;
		}
	}
}
