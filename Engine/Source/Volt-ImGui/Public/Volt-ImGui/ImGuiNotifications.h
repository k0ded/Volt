#pragma once

#include "Volt-ImGui/Config.h"

#include <functional>
#include <string>

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
		std::string title;
		std::string message;

		int32_t dismissTime;

		std::function<void()> onButtonPress;
		std::string buttonLabel;
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
