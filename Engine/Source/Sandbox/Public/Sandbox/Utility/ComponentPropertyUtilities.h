#pragma once

#include <AssetSystem/AssetType.h>

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/TypeTraits/TypeIndex.h>

#include <unordered_map>
#include <functional>

namespace Volt
{
	class IComponentTypeDesc;
	class IEnumTypeDesc;
	class IArrayTypeDesc;
	class Entity;
	class Scene;
	struct ComponentMember;
}

class ComponentPropertyUtility
{
public:
	static void DrawComponents(Volt::Scene& scene, Volt::Entity entity);

private:
	static void Initialize();

	static bool DrawComponent(Volt::Scene& scene, Volt::Entity entity, const Volt::IComponentTypeDesc* componentType, void* data, const size_t offset, bool isOpen, bool isSubSection);
	static bool DrawComponentDefaultMember(Volt::Scene& scene, Volt::Entity entity, const Volt::ComponentMember& member, void* data, const size_t offset);
	static bool DrawComponentDefaultMemberArray(Volt::Scene& scene, Volt::Entity entity, const Volt::ComponentMember& arrayMember, void* elementData, const size_t index, const TypeTraits::TypeIndex& typeIndex, AssetType arrayAssetType);
	static bool DrawComponentEnum(Volt::Scene& scene, Volt::Entity entity, const Volt::ComponentMember& member, const Volt::IEnumTypeDesc* enumType, void* data, const size_t offset);
	static bool DrawComponentArray(Volt::Scene& scene, Volt::Entity entity, const Volt::ComponentMember& member, const Volt::IArrayTypeDesc* arrayDesc, void* data, const size_t offset);

	static void AddLocalChangeToEntity(Volt::Scene& scene, Volt::Entity entity, const VoltGUID& componentGuid, uint32_t memberIdentifier);

	inline static bool s_initialized = false;
	inline static std::unordered_map<TypeTraits::TypeIndex, std::function<bool(std::string_view, void*, const size_t)>> s_propertyFunctions;

	ComponentPropertyUtility() = delete;
};
