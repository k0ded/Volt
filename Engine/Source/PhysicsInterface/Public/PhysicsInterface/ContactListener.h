#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Array.h>

namespace Volt
{
	class PhysicsActor;
	
	enum class PhysicsContactType : uint8_t
	{
		FirstTouch,
		LostTouch
	};

	enum class PhysicsTriggerType : uint8_t
	{
		EnterTrigger,
		ExitTrigger
	};

	struct ContactHeader
	{
		Array<Ref<PhysicsActor>, 2> actors;
		PhysicsContactType contactType;
	};

	struct TriggerHeader
	{
		Ref<PhysicsActor> triggerActor;
		Ref<PhysicsActor> otherActor;
		PhysicsTriggerType triggerType;
	};

	class ContactListener
	{
	public:
		virtual void OnWake(const Vector<PhysicsActor*>& actors) = 0;
		virtual void OnSleep(const Vector<PhysicsActor*>& actors) = 0;
		virtual void OnContact(const ContactHeader& contactHeader) = 0;
		virtual void OnTrigger(const TriggerHeader& triggerHeader) = 0;
	};
}
