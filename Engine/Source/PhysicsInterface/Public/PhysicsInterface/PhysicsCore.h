#pragma once

namespace Volt
{
	class ContactListener;

	struct PhysicsCoreCreateInfo
	{
		bool allowDebugging = false;

		Ref<ContactListener> contactListener;
	};

	class PhysicsCore
	{
	public:
		virtual ~PhysicsCore() {}

	private:
	};
}
