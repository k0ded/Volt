#pragma once

#include "Volt-Scene/Config.h"
#include <EntitySystem/Entity.h>



namespace Volt
{
	class Scene;

	VTS_API void  CopyEntity(Entity srcEntity, Entity dstEntity, std::set<VoltGUID> componentsToSkip = {});
	VTS_API Entity DuplicateEntity(Entity srcEntity, Scene& targetScene, Entity parent = Entity::Null(), std::set<VoltGUID> componentsToSkip = {});
	VTS_API void CopyComponent(const uint8_t* srcData, uint8_t* dstData, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity);

	template<typename ...T>
	inline std::set<VoltGUID> CreateSkipComponentOnCopySet()
	{
		return { GetTypeGUID<T>()... };
	}
}
