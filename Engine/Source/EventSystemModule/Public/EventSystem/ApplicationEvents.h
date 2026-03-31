#pragma once

#include "EventSystem/Event.h"

namespace Volt
{
	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent(float timestep)
			: m_timestep(timestep)
		{
		}

		VT_INLINE float GetTimestep() const { return m_timestep; }

		EVENT_CLASS(AppUpdateEvent, "{07AB89BC-1ACE-4FFD-B2C3-ED6DA5A24029}"_guid);

	private:
		float m_timestep;
	};

	// #Deprecate
	class AppTickEvent : public Event
	{
	public:
		AppTickEvent(float timestep, uint64_t frameIndex)
			: m_timestep(timestep), m_frameIndex(frameIndex)
		{}

		VT_INLINE float GetTimestep() const { return m_timestep; }
		VT_INLINE uint64_t GetFrameIndex() const { return m_frameIndex; }

		EVENT_CLASS(AppTickEvent, "{79E9D276-C6CB-4D70-AB1B-A27953BA1FF1}"_guid);

	private:
		float m_timestep;
		uint64_t m_frameIndex;
	};

	class AppPostFrameUpdateEvent : public Event
	{
	public:
		AppPostFrameUpdateEvent(float timestep)
			: m_timestep(timestep)
		{ }

		VT_INLINE float GetTimestep() const { return m_timestep; }

		EVENT_CLASS(AppPostFrameUpdateEvent, "{FD1AD6FA-5428-4FBD-BDF9-7D1BFF009D85}"_guid);
	private:
		float m_timestep;
	};

	class AppImGuiUpdateEvent : public Event
	{
	public:
		AppImGuiUpdateEvent() = default;

		EVENT_CLASS(AppImGuiUpdateEvent, "{AAFC6679-2596-4706-99B1-E8F27A339C55}"_guid);
	};

	class AppPreRenderEvent : public Event
	{
	public:
		AppPreRenderEvent(uint64_t frameIndex)
			: m_frameIndex(frameIndex)
		{}

		VT_INLINE uint64_t GetFrameIndex() const { return m_frameIndex; }

		EVENT_CLASS(AppPreRenderEvent, "{F1179BDD-7FF6-4CD1-9238-A1BC31E7A062}"_guid);
	
	private:
		uint64_t m_frameIndex;
	};

	class AppImGuiBlockingUpdateEvent : public Event
	{
	public:
		AppImGuiBlockingUpdateEvent() = default;

		EVENT_CLASS(AppImGuiBlockingUpdateEvent, "{00E0C06F-55C3-43CF-BE76-302E1B699556}"_guid)
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent(float timestep)
			: m_timestep(timestep)
		{}

		VT_INLINE float GetTimestep() { return m_timestep; }

		EVENT_CLASS(AppRenderEvent, "{B38DBA07-ACAA-4334-8751-2570560D4490}"_guid);

	private:
		float m_timestep;
	};

	// #Deprecate
	class AppBeginFrameEvent : public Event
	{
	public:
		AppBeginFrameEvent()
		{}

		EVENT_CLASS(AppBeginFrameEvent, "{E7899035-D479-4F85-A101-F7396EC89A61}"_guid);
	};

	// #Deprecate
	class AppPresentFrameEvent : public Event
	{
	public:
		AppPresentFrameEvent()
		{}

		EVENT_CLASS(AppPresentFrameEvent, "{4DA476C4-E0AA-42C6-A66D-55B57E985959}"_guid);
	};
}
