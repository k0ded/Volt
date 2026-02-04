#pragma once

#include "Sandbox/ComponentVisualizers/ComponentVisualizer.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterface.h"

#include <EntitySystem/ComponentReflection.h>
#include <EntitySystem/Entity.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/VoltGUID.h>

template<typename ClassType, typename ComponentType>
concept HasDrawVisualizationFunc = requires(ClassType t, EditorDrawInterface& edi, ComponentType& c, Volt::Entity e)
{
	t.DrawVisualization(edi, c, e);
};

class ComponentVisualizerRegistry
{
public:
	struct ComponentVisualizerRegistrationInfo
	{
		Ref<BaseComponentVisualizer> componentVisualizer;
		std::function<void(Ref<BaseComponentVisualizer> visualizer, EditorDrawInterface&, Volt::Entity)> drawVisualizationFunc;
	};

	template<typename ComponentVisualizerType, typename ComponentType>
	void RegisterComponentVisualizer()
	{
		const VoltGUID componentGUID = Volt::GetTypeGUID<ComponentType>();
		VT_ENSURE(!m_componentVisualizerRegistrationInfo.contains(componentGUID));
	
		ComponentVisualizerRegistrationInfo& registrationInfo = m_componentVisualizerRegistrationInfo[componentGUID];
		registrationInfo.componentVisualizer = CreateRef<ComponentVisualizerType>();

		if constexpr (HasDrawVisualizationFunc<ComponentVisualizerType, ComponentType>)
		{
			registrationInfo.drawVisualizationFunc = [](Ref<BaseComponentVisualizer> visualizer, EditorDrawInterface& edi, Volt::Entity entity) 
			{
				Ref<ComponentVisualizerType> typedVisualizer = std::reinterpret_pointer_cast<ComponentVisualizerType>(visualizer);
				typedVisualizer->DrawVisualization(edi, entity.GetComponent<ComponentType>(), entity);
			};
		}
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

	VT_INLINE Ref<BaseComponentVisualizer> GetComponentVisualizer(const VoltGUID& componentGuid)
	{
		return m_componentVisualizerRegistrationInfo.at(componentGuid).componentVisualizer;
	}

	void DrawVisualizationForComponent(const VoltGUID& componentGuid, EditorDrawInterface& edi, Volt::Entity entity)
	{
		auto it = m_componentVisualizerRegistrationInfo.find(componentGuid);
		if (it != m_componentVisualizerRegistrationInfo.end())
		{
			const ComponentVisualizerRegistrationInfo& info = it->second;

			if (info.drawVisualizationFunc)
			{
				info.drawVisualizationFunc(info.componentVisualizer, edi, entity);
			}
		}
	}

	static ComponentVisualizerRegistry& Get();

private:
	Map<VoltGUID, ComponentVisualizerRegistrationInfo> m_componentVisualizerRegistrationInfo;
};

// Must lie in a compilation unit (cpp file)
#define VT_REGISTER_COMPONENT_VISUALIZER(componentVisualizerType) \
	class ComponentVisualizerRegistrar_##componentVisualizerType \
	{ \
	public: \
		VT_INLINE ComponentVisualizerRegistrar_##componentVisualizerType() \
		{ \
			using ComponentType = typename componentVisualizerType::ComponentType; \
			ComponentVisualizerRegistry::Get().RegisterComponentVisualizer<componentVisualizerType, ComponentType>(); \
		} \
		VT_INLINE ~ComponentVisualizerRegistrar_##componentVisualizerType() \
		{ \
			using ComponentType = typename componentVisualizerType::ComponentType; \
			ComponentVisualizerRegistry::Get().UnregisterComponentVisualizer<ComponentType>(); \
		} \
	} g_componentVisualizerRegistrar_##componentVisualizerType
