#include "sbpch.h"

#include "Sandbox/Utility/UIPropertiesExtension.h"

#include <Volt-Scene/Scene.h>
#include <EntitySystem/Entity.h>

#include <EntitySystem/Scripting/CoreComponents.h>

namespace UI
{

	bool PropertyEntity(const String& text, Volt::Scene& scene, Volt::EntityID& value, const String& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		String id = MakePropertyID();

		Volt::Entity entity = scene.GetEntityFromID(value);

		String entityName;
		if (entity)
		{
			entityName = entity.GetComponent<Volt::TagComponent>().tag;
		}
		else
		{
			entityName = "Null";
		}

		changed = DrawItem([&]()
		{
			return ImGui::InputText(id.c_str(), &entityName, ImGuiInputTextFlags_ReadOnly);
		});

		changed = DragDropTarget<Volt::EntityID>("scene_entity_hierarchy", value);
		EndPropertyRow();

		return changed;
	}

	bool PropertyEntity(Volt::Scene& scene, Volt::EntityID& value, const float width, const String& toolTip)
	{
		bool changed = false;

		SimpleToolTip(toolTip);
		String id = MakePropertyID();

		Volt::Entity entity = scene.GetEntityFromID(value);

		String entityName;
		if (entity)
		{
			entityName = entity.GetComponent<Volt::TagComponent>().tag;
		}
		else
		{
			entityName = "Null";
		}

		changed = DrawItem(width, [&]()
		{
			return ImGui::InputText(id.c_str(), &entityName, ImGuiInputTextFlags_ReadOnly);
		});

		changed = DragDropTarget<Volt::EntityID>("scene_entity_hierarchy", value);

		EndPropertyRow();

		return changed;
	}

}
