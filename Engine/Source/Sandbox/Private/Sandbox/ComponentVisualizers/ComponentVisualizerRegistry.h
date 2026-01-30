#pragma once

#include "Sandbox/ComponentVisualizers/ComponentVisualizer.h"

#include <EntitySystem/ComponentReflection.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/VoltGUID.h>

class ComponentVisualizerRegistry
{
public:
	struct ComponentVisualizerRegistrationInfo
	{
		Ref<ComponentVisualizer> componentVisualizer;
	};

	template<typename ComponentVisualizerType, typename ComponentType>
	void RegisterComponentVisualizer()
	{
		const VoltGUID componentGUID = Volt::GetTypeGUID<ComponentType>();
		VT_ENSURE(!m_componentVisualizerRegistrationInfo.contains(componentGUID));
	
		ComponentVisualizerRegistrationInfo& registrationInfo = m_componentVisualizerRegistrationInfo[componentGUID];
		registrationInfo.componentVisualizer = CreateRef<ComponentVisualizerType>();
	}

	template<typename ComponentType>
	void UnregisterComponentVisualizer()
	{
		const VoltGUID componentGUID = Volt::GetTypeGUID<ComponentType>();

		if (m_componentVisualizerRegistrationInfo.contains(componentGUID))
		{
			m_componentVisualizerRegistrationInfo.erase(componentGUID);
		}
	}

	VT_INLINE bool HasComponentVisualizer(const VoltGUID& componentGuid)
	{
		return m_componentVisualizerRegistrationInfo.contains(componentGuid);
	}

	VT_INLINE Ref<ComponentVisualizer> GetComponentVisualizer(const VoltGUID& componentGuid)
	{
		return m_componentVisualizerRegistrationInfo.at(componentGuid).componentVisualizer;
	}

	static ComponentVisualizerRegistry& Get();

private:
	Map<VoltGUID, ComponentVisualizerRegistrationInfo> m_componentVisualizerRegistrationInfo;
};

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_COMPONENT_VISUALIZER(componentVisualizerType, componentType) \
	class ComponentVisualizerRegistrar_##componentVisualizerType \
	{ \
	public: \
		VT_INLINE ComponentVisualizerRegistrar_##componentVisualizerType() \
		{ \
			ComponentVisualizerRegistry::Get().RegisterComponentVisualizer<componentVisualizerType, componentType>(); \
		} \
		VT_INLINE ~ComponentVisualizerRegistrar_##componentVisualizerType() \
		{ \
			ComponentVisualizerRegistry::Get().UnregisterComponentVisualizer<componentType>(); \
		} \
	} g_componentVisualizerRegistrar_##componentVisualizerType
