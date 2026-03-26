#pragma once

#include <CoreUtilities/Pointers/Ref.h>

namespace Volt
{
	class ContactListener;
	class PhysicsScene;
	class PhysicsMaterial;

	struct PhysicsSceneCreateInfo;
	struct PhysicsMaterialCreateInfo;

	struct PhysicsCoreCreateInfo
	{
		bool allowDebugging = false;

		Ref<ContactListener> contactListener;
	};

	class PhysicsCore
	{
	public:
		virtual ~PhysicsCore() {}
		virtual Ref<PhysicsScene> CreateScene(const PhysicsSceneCreateInfo& createInfo) const = 0;
		virtual Ref<PhysicsMaterial> CreateMaterial(const PhysicsMaterialCreateInfo& createInfo) const = 0;

	private:
	};
}
