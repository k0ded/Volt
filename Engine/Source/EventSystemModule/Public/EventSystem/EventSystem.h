#pragma once

#include "EventSystem/Event.h"

#include <SubSystem/SubSystem.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/VoltGUID.h>

#include <shared_mutex>
namespace Volt
{
	class EventListener;
	class EVENTMODULE_API EventSystem : public SubSystem
	{
	public:
		EventSystem();
		~EventSystem();

		typedef std::function<bool(Event& e)> EventListenerDelegate;
		typedef std::function<bool()> EventDispatchPredicate;

		static void RegisterListener(VoltGUID eventGUID, EventListenerDelegate delegate, EventDispatchPredicate predicate, EventListener* listener);
		static void UnregisterListener(VoltGUID eventGUID, EventListener* listener);
		static void UnregisterListeners(EventListener* listener);

		static void Update();

		template<IsEvent T>
		static void DispatchEvent(T& e)
		{
			VT_ENSURE(s_instance);
			s_instance->DispatchEventInternal(T::GetStaticGUID(), e);
		}

		VT_DECLARE_SUBSYSTEM("{53104069-97D1-459F-B307-7E6DB62676BF}"_guid)

	private:
		inline static EventSystem* s_instance = nullptr;
	
		struct ListenerInfo
		{
			EventListener* listener;
			EventListenerDelegate delegate;
			EventDispatchPredicate predicate;
			bool Invalid = false;
		};

		void DispatchEventInternal(VoltGUID eventGUID, Event& e);

		Map<VoltGUID, Vector<ListenerInfo>> m_registeredListeners;
		Map<VoltGUID, Vector<ListenerInfo>> m_queuedRegisters;
		Map<VoltGUID, Vector<int32_t>> m_queuedUnregisters;

		std::shared_mutex m_registerQueueMutex;
		std::shared_mutex m_unregisterQueueMutex;
		std::shared_mutex m_listenersMutex;
	};
}
