#pragma once

#include "Volt-ImGui/Config.h"

#include <CoreUtilities/String/VoltString.h>

#include <functional>

enum class ImGuiToastType : uint8_t;

namespace Volt
{
	enum ImGuiNotificationType
	{
		None,
		Info,
		Warning,
		Error,
		Success
	};

	struct ImGuiNotificationInfo
	{
		ImGuiNotificationType type;
		String title;
		String message;

		int32_t dismissTime;

		std::function<void()> onButtonPress;
		String buttonLabel;
	};


	class ImGuiNotifications
	{
	public:
		VTIMGUI_API static void InsertNotification(const ImGuiNotificationInfo& notifInfo);
		
		static void RenderNotifications();
	private:
		static ImGuiToastType ToastTypeFromNotificationType(ImGuiNotificationType type);
	};
}
