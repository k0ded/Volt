#pragma once

#include "EventSystem/Config.h"

#include <CoreUtilities/VoltGUID.h>

#define EVENT_CLASS(eventClass, eventGUID) static VoltGUID GetStaticGUID() {return eventGUID;}\
										virtual const VoltGUID GetGUID() const override {return GetStaticGUID(); }\
										virtual const char* GetName() const override { return #eventClass; }

#define VT_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

namespace Volt
{
	class EVENTMODULE_API Event
	{
	public:
		virtual const VoltGUID GetGUID() const = 0;
		virtual const char* GetName() const = 0;
		virtual String ToString() const { return GetName(); }

		inline bool IsHandled() { return m_handled; }
		inline void SetHandled(bool handled) { m_handled = handled; }
	private:
		bool m_handled = false;
	};

	template<typename T>
	concept IsEvent = std::is_base_of_v<Event, T>;
}
