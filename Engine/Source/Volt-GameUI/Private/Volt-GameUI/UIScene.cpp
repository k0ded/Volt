#include "vtguipch.h"

#include "Volt-GameUI/UIScene.h"
#include "Volt-GameUI/UIWidget.h"
#include "Volt-GameUI/UIComponents.h"

#include <CoreUtilities/Time/TimeUtility.h>

namespace Volt
{
	UIWidget UIScene::CreateWidget(const std::string& tag)
	{
		entt::entity handle = m_registry.create();
		
		UIWidget newWidget = UIWidget(handle, this);
		newWidget.AddComponent<UITransformComponent>();
		
		auto& idComp = newWidget.AddComponent<UIIDComponent>();
		idComp.timeCreateID = TimeUtility::GetTimeSinceEpoch();
	
		// Tag
		{
			auto& tagComp = newWidget.AddComponent<UITagComponent>();
			if (tag.empty())
			{
				tagComp.tag = "New Tag";
			}
			else
			{
				tagComp.tag = tag;
			}
		}


		m_uuidToEntityMap[idComp.id] = handle;
		return newWidget;
	}

	UIWidget UIScene::GetWidgetFromUUID(const EntityID uuid) const
	{
		if (!m_uuidToEntityMap.contains(uuid))
		{
			return UIWidget::Null();
		}

		return UIWidget(m_uuidToEntityMap.at(uuid), const_cast<UIScene*>(this));
	}
}
