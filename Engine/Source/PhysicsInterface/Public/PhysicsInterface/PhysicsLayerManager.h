#pragma once

#include "PhysicsInterface/Config.h"
#include "PhysicsInterface/PhysicsLayer.h"

#include <CoreUtilities/Core.h>

namespace Volt
{
	class VTPI_API PhysicsLayerManager
	{
	public:
		bool AreLayersColliding(PhysicsLayerID layerA, PhysicsLayerID layerB);
		const PhysicsLayer& GetLayer(PhysicsLayerID layerId) const;

	private:
		Vector<PhysicsLayer> m_layers;
	};
}

extern VTPI_API Volt::PhysicsLayerManager g_physicsLayerManager;

VT_INLINE Volt::PhysicsLayerManager& GetPhysicsLayerManager()
{
	return g_physicsLayerManager;
}
