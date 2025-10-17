#pragma once
#include "AssetSystem/Config.h"
#include "AssetSystem/AssetHandle.h"


#include <EventSystem/Event.h>

namespace Volt
{
	//sent when an asset is created and added to the asset manager
	class VTAS_API AssetCreatedEvent : public Event
	{
	public:
		AssetCreatedEvent(AssetHandle handle)
			: m_assetHandle(handle)
		{}

		VT_NODISCARD VT_INLINE const AssetHandle& GetAssetHandle() { return m_assetHandle; }

		EVENT_CLASS(AssetCreatedEvent, "{23D5D481-DDD8-4772-B50D-0DEAFC5429F3}"_guid);
	private:
		AssetHandle m_assetHandle;
	};

	//sent when the file for an asset is created
	class VTAS_API AssetSavedEvent : public Event
	{
	public:
		AssetSavedEvent(AssetHandle handle)
			: m_assetHandle(handle)
		{}

		VT_NODISCARD VT_INLINE const AssetHandle& GetAssetHandle() { return m_assetHandle; }

		EVENT_CLASS(AssetSavedEvent, "{E191199A-FA07-4D61-A63F-E77FBC504FA4}"_guid);
	private:
		AssetHandle m_assetHandle;
	};
}
