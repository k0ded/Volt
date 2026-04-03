#include "eventpch.h"
#include "EventSystem.h"

#include "EventSystem/EventListener.h"

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/ConsoleVariableRegistry.h>
#include <CoreUtilities/ThreadConfig.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(EventSystem, Minimal, PreEngine);

	static ConsoleVariable<int32_t> g_eventSystemLogAllEvents(
		"es.LogAllEvents",
		0,
		"Whether or not to log all events that pass through the EventSystem."
	);

	EventSystem::EventSystem()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	}

	EventSystem::~EventSystem()
	{
		s_instance = nullptr;
	}

	void EventSystem::RegisterListener(VoltGUID eventGUID, EventListenerDelegate delegate, EventDispatchPredicate predicate, EventListener* listener)
	{
		VT_ENSURE(s_instance);
		std::shared_lock<std::shared_mutex> lock(s_instance->m_dispatchSetMutex);

		//if we are in the process of dispatching the event that is trying to be registered to, queue the register instead
		if (s_instance->m_dispatchSet.contains(eventGUID))
		{
			s_instance->m_queuedRegisters[eventGUID].emplace_back(listener, delegate, predicate);
		}
		else
		{
			s_instance->m_registeredListeners[eventGUID].emplace_back(listener, delegate, predicate);
		}
	}

	void EventSystem::Update()
	{
		VT_ASSERT_MSG(Threads::GetThreadConfig().isMainThread, "EventSystem Update may only be called on the main thread.");

		if (s_instance->m_queuedUnregisters.empty() && s_instance->m_queuedRegisters.empty())
		{
			return;
		}

		//only process unregisters if we are currently not in an event
		bool processUnregister = true;
		{
			std::shared_lock<std::shared_mutex> lock(s_instance->m_dispatchSetMutex);
			processUnregister = s_instance->m_dispatchSet.empty();
		}

		if (processUnregister)
		{
			for (auto& [guid, indices] : s_instance->m_queuedUnregisters)
			{
				std::sort(indices.begin(), indices.end());
				for (int32_t i = static_cast<int32_t>(indices.size() - 1); i >= 0; i--)
				{
					const size_t removeIndex = indices[i];
					const auto removeIt = s_instance->m_registeredListeners[guid].begin() + removeIndex;
					s_instance->m_registeredListeners[guid].erase_unsorted(removeIt);
				}
			}
			s_instance->m_queuedUnregisters.clear();
		}

		for (auto& [guid, infos] : s_instance->m_queuedRegisters)
		{
			for (const ListenerInfo& info : infos)
			{
				s_instance->m_registeredListeners[guid].push_back(info);
			}
		}


		s_instance->m_queuedRegisters.clear();
	}

	void EventSystem::UnregisterListener(VoltGUID eventGUID, EventListener* listener)
	{
		auto& listeners = s_instance->m_registeredListeners[eventGUID];

		const bool notOnMainThread = !Threads::GetThreadConfig().isMainThread;
		for (int32_t i = static_cast<int32_t>(listeners.size()) - 1; i >= 0; --i)
		{
			if (listeners[i].listener == listener)
			{
				std::unique_lock<std::shared_mutex> lock(*listeners[i].dispatchMutex, std::try_to_lock);

				//if we failed to lock while on main thread, we just let it pass through and queue the unregister
				// this is because it means, a listener that got a dispatch unregistered itself
				//if we are not on main thread, block until unlocked
				if (!lock.owns_lock() && notOnMainThread)
				{
					lock.lock();
				}

				s_instance->m_queuedUnregisters[eventGUID].push_back(i);
				listeners[i].invalid = true;
				break;
			}
		}
	}

	void EventSystem::UnregisterListeners(EventListener* listener)
	{
		VT_ENSURE(s_instance);

		for (auto& [guid, delegates] : s_instance->m_registeredListeners)
		{
			auto it = std::find_if(delegates.begin(), delegates.end(), [&](const ListenerInfo& info)
			{
				return info.listener == listener;
			});

			if (it != delegates.end())
			{
				std::unique_lock<std::shared_mutex> lock(*it->dispatchMutex);
				const int32_t index = static_cast<int32_t>(it - delegates.begin());
				s_instance->m_queuedUnregisters[guid].push_back(index);
				it->invalid = true;
			}
		}
	}

	void EventSystem::DispatchEventInternal(VoltGUID eventGUID, Event& e)
	{
		VT_PROFILE_SCOPE(std::format("Dispatch {}", e.GetName()).c_str());

		VT_ASSERT_MSG(Threads::GetThreadConfig().isMainThread, "Event was dispatched from a thread other than the Main Thread.");
		VT_ASSERT_MSG(!m_dispatchSet.contains(eventGUID), "Recursive event call detected, this is not allowed!");

		{
			std::unique_lock<std::shared_mutex> lock(s_instance->m_dispatchSetMutex);
			m_dispatchSet.insert(eventGUID);
		}

		if (g_eventSystemLogAllEvents.GetValue())
		{
			VT_LOGC(Trace, LogEventSystem, "Dispatched event: {}", e.ToString());
		}

		//do event dispatch
		for(int32_t i = static_cast<int32_t>(m_registeredListeners[eventGUID].size()) -1; i >= 0; i--)
		{
			ListenerInfo& info = m_registeredListeners[eventGUID][i];
			//lock this info so that we dont unregister it while dispatching for it
			std::unique_lock<std::shared_mutex> lock(*info.dispatchMutex, std::try_to_lock);
			//if the lock failed to take ownership, this info is currently being unregistered
			if (!lock.owns_lock())
			{
				continue;
			}

			//the listener has already been unregistered
			if (info.invalid)
			{
				continue;
			}

			if (info.listener->AreEventsBlocked())
			{
				continue;
			}

			//a listener gets to determine if it wants to recieve the event or not
			if (info.predicate)
			{
				if (!info.predicate())
				{
					continue;
				}
			}

			// If the event gets handled, we skip the rest of the listeners
			if (info.delegate)
			{
				if (info.delegate(e))
				{
					break;
				}
			}
		}
		{
			std::unique_lock<std::shared_mutex> lock(s_instance->m_dispatchSetMutex);
			m_dispatchSet.erase(eventGUID);
		}
	}
}
