
#pragma once

#include <AssetSystem/AssetTypes.h>

#include <AssetSystem/Asset.h>

#include <EntitySystem/EntityID.h>
#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/ECSAccessBuilder.h>

#include <EntitySystem/Scripting/CommonComponent.h>
#include <EntitySystem/Scripting/CoreComponents.h>

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace Volt
{
	struct PrefabComponentLocalChange
	{
		VoltGUID componentGUID = VoltGUID::Null();
		uint32_t memberIdentifier;
		
		VT_INLINE friend Archive& operator<<(Archive& archive, PrefabComponentLocalChange& value)
		{
			archive << value.componentGUID;
			archive << value.memberIdentifier;
			return archive;
		}
	};

	struct PrefabComponent
	{
		AssetHandle prefabAsset = Asset::Null();
		EntityID prefabEntity = EntityID(0);
		EntityID sceneRootEntity = EntityID(0);
		uint32_t version = 0;

		Vector<PrefabComponentLocalChange> componentLocalChanges;

		bool isDirty = false;

		static void ReflectType(TypeDesc<PrefabComponent>& reflect)
		{
			reflect.SetGUID("{B8A83ACF-F1CA-4C9F-8D1E-408B5BB388D2}"_guid);
			reflect.SetLabel("Prefab Component");
			reflect.SetHidden();
			reflect.AddMember(&PrefabComponent::prefabAsset, 'prea', "Prefab Asset", "", Asset::Null(), AssetTypes::Prefab);
			reflect.AddMember(&PrefabComponent::prefabEntity, 'pree', "Prefab Entity", "", EntityID(0));
			reflect.AddMember(&PrefabComponent::sceneRootEntity, 'sre', "Scene Root Entity", "", EntityID(0));
			reflect.AddMember(&PrefabComponent::version, 'ver', "Version", "", 0);
			reflect.AddMember(&PrefabComponent::componentLocalChanges, 'clc', "Component Local Changes", "", PrefabComponentLocalChange());
		}

		REGISTER_COMPONENT(PrefabComponent);
	};
}
