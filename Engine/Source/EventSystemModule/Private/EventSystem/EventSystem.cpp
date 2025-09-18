#include "eventpch.h"
#include "EventSystem.h"

#include "EventSystem/EventListener.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(EventSystem, Minimal, PreEngine, -1);

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
		s_instance->m_registeredListeners[eventGUID].emplace_back(listener, delegate, predicate);
	}

	void EventSystem::Update()
	{
		if (s_instance->m_queuedUnregisters.empty())
		{
			return;
		}

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

	void EventSystem::UnregisterListener(VoltGUID eventGUID, EventListener* listener)
	{
		auto& listeners = s_instance->m_registeredListeners[eventGUID];

		for (int32_t i = static_cast<int32_t>(listeners.size()) - 1; i >= 0; --i)
		{
			if (listeners[i].listener == listener)
			{
				s_instance->m_queuedUnregisters[eventGUID].push_back(i);
				listeners[i].Invalid = true;
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
				const int32_t index = static_cast<int32_t>(it - delegates.begin());
				s_instance->m_queuedUnregisters[guid].push_back(index);
				it->Invalid= true;
			}
		}
	}

	void EventSystem::DispatchEventInternal(VoltGUID eventGUID, Event& e)
	{
		VT_PROFILE_SCOPE(std::format("Dispatch {}", e.GetName()).c_str());

		for (auto& info : m_registeredListeners[eventGUID])
		{
			if (info.Invalid)
			{
				continue;
			}

			if (info.listener->AreEventsBlocked())
			{
				continue;
			}

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
	}
}
