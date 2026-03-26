#pragma once

#include "EntitySystem/ComponentRegistry.h"
#include "EntitySystem/EntityID.h"

#include <CoreUtilities/EnumUtils.h>

#include <glm/glm.hpp>

namespace Volt
{
	enum class Movability : uint32_t
	{
		Static = 0,
		Stationary,
		Movable
	};

	static void ReflectType(TypeDesc<Movability>& reflect)
	{
		reflect.SetGUID("{2DC55D4C-63C8-432A-8037-1D6762C8DC33}"_guid);
		reflect.SetLabel("Movability");
		reflect.SetDefaultValue(Movability::Static);
		reflect.AddConstant(Movability::Static, "static", "Static");
		reflect.AddConstant(Movability::Stationary, "stationary", "Stationary");
		reflect.AddConstant(Movability::Movable, "movable", "Movable");
	}

	VT_SETUP_ENUM_SERIALIZE_OPERATOR(Movability);

	struct VTES_API TagComponent
	{
		String tag;

		static void ReflectType(TypeDesc<TagComponent>& reflect)
		{
			reflect.SetGUID("{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid);
			reflect.SetLabel("Tag Component");
			reflect.SetHidden();
			reflect.AddMember(&TagComponent::tag, 'tag', "Tag", "", String(""));
		}
	};

	struct VTES_API IDComponent
	{
		EntityID id{};

		static void ReflectType(TypeDesc<IDComponent>& reflect)
		{
			reflect.SetGUID("{663E0E0B-43EC-4973-8A9B-FF8A0BA566AA}"_guid);
			reflect.SetLabel("ID Component");
			reflect.SetHidden();
			reflect.AddMember(&IDComponent::id, 'id', "ID", "", EntityID{}, ComponentMemberFlag::NoSerialize);
		}
	};

	struct VTES_API TransformComponent
	{
		glm::vec3 position = { 0.f };
		glm::quat rotation = glm::identity<glm::quat>();
		glm::vec3 scale = { 1.f };

		Movability movability = Movability::Static;

		bool visible = true;
		bool locked = false;

		inline const glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.f), position) *
				glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.f), scale);
		}

		inline const glm::vec3 GetForward() const
		{
			return glm::rotate(rotation, glm::vec3{ 0.f, 0.f, 1.f });
		}

		inline const glm::vec3 GetRight() const
		{
			return glm::rotate(rotation, glm::vec3{ 1.f, 0.f, 0.f });
		}

		inline const glm::vec3 GetUp() const
		{
			return glm::rotate(rotation, glm::vec3{ 0.f, 1.f, 0.f });
		}

		static void ReflectType(TypeDesc<TransformComponent>& reflect)
		{
			reflect.SetGUID("{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid);
			reflect.SetLabel("Transform Component");
			reflect.SetHidden();
			reflect.AddMember(&TransformComponent::position, 'pos', "Position", "", glm::vec3{0.f});
			reflect.AddMember(&TransformComponent::rotation, 'rot', "Rotation", "", glm::identity<glm::quat>());
			reflect.AddMember(&TransformComponent::scale, 'scal', "Scale", "", glm::vec3{1.f});
			reflect.AddMember(&TransformComponent::visible, 'vis', "Visible", "", true);
			reflect.AddMember(&TransformComponent::locked, 'lock', "Locked", "", false);
			reflect.AddMember(&TransformComponent::movability, 'mvbl', "Movability", "", Movability::Static);
		}
	};

	struct VTES_API RelationshipComponent
	{
		EntityID parent = EntityID(0);
		Vector<EntityID> children;

		static void ReflectType(TypeDesc<RelationshipComponent>& reflect)
		{
			reflect.SetGUID("{4A5FEDD2-4D0B-4696-A9E6-DCDFFB25B32C}"_guid);
			reflect.SetLabel("Relationship Component");
			reflect.SetHidden();
			reflect.AddMember(&RelationshipComponent::parent, 'par', "Parent", "", EntityID(0));
			reflect.AddMember(&RelationshipComponent::children, 'chld', "Children", "", EntityID(0));
		}
	};
}
