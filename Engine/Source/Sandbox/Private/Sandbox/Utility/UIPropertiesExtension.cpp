#include "sbpch.h"

#include "Sandbox/Utility/UIPropertiesExtension.h"

#include <Volt-Scene/Scene.h>
#include <Volt-Scene/Entity.h>

#include <EntitySystem/Scripting/CoreComponents.h>

namespace UI
{

	bool PropertyEntity(const std::string& text, Weak<Volt::Scene> scene, Volt::EntityID& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		Volt::Entity entity = scene->GetEntityFromID(value);

		std::string entityName;
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
			return ImGui::InputTextString(id.c_str(), &entityName, ImGuiInputTextFlags_ReadOnly);
		});

		if (auto ptr = DragDropTarget("scene_entity_hierarchy"))
		{
			Volt::EntityID entityId = *(Volt::EntityID*)ptr;
			value = entityId;
			changed = true;
		}

		EndPropertyRow();

		return changed;
	}

	bool PropertyEntity(Weak<Volt::Scene> scene, Volt::EntityID& value, const float width, const std::string& toolTip)
	{
		bool changed = false;

		SimpleToolTip(toolTip);
		std::string id = MakePropertyID();

		Volt::Entity entity = scene->GetEntityFromID(value);

		std::string entityName;
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
			return ImGui::InputTextString(id.c_str(), &entityName, ImGuiInputTextFlags_ReadOnly);
		});

		if (auto ptr = DragDropTarget("scene_entity_hierarchy"))
		{
			Volt::EntityID entityId = *(Volt::EntityID*)ptr;
			value = entityId;
			changed = true;
		}

		EndPropertyRow();

		return changed;
	}

}
