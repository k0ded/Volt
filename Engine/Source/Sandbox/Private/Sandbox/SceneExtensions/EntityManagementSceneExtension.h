#pragma once

#include <Volt-Scene/SceneExtension.h>

class EntityManagementSceneExtension : public Volt::SceneExtension
{
public:
	~EntityManagementSceneExtension() override = default;

	void OnEntityCreated(Volt::Entity entity) override;
	void OnEntityDestroyed(Volt::EntityID entityId) override;

private:

};
