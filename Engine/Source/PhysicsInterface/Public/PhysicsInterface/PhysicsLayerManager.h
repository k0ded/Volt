#pragma once

#include "PhysicsInterface/Config.h"
#include "PhysicsInterface/PhysicsLayer.h"

#include <CoreUtilities/Core.h>

namespace Volt
{
	class VTPI_API PhysicsLayerManager
	{
	public:
		void AddLayer(const std::string& name);
		void SetLayerCollision(uint32_t layer, uint32_t otherLayer, bool shouldCollide);
		
		bool AreLayersColliding(PhysicsLayerID layerA, PhysicsLayerID layerB);
		const PhysicsLayer& GetLayer(PhysicsLayerID layerId) const;
		PhysicsLayer& GetLayer(PhysicsLayerID layerId);

	private:
		uint32_t GetNextLayerID() const;

		Vector<PhysicsLayer> m_layers;
	};
}

extern VTPI_API Volt::PhysicsLayerManager g_physicsLayerManager;

VT_INLINE Volt::PhysicsLayerManager& GetPhysicsLayerManager()
{
	return g_physicsLayerManager;
}
