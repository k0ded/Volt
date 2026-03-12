#include "ImGuiNotifications.h"

#include <ImGuiNotify.hpp>

namespace Volt
{
	void ImGuiNotifications::InsertNotification(const ImGuiNotificationInfo& notifInfo)
	{
		ImGuiToast toast(ToastTypeFromNotificationType(notifInfo.type), notifInfo.dismissTime);
		toast.setTitle(notifInfo.title.c_str());
		toast.setContent(notifInfo.message.c_str());

		toast.setOnButtonPress(notifInfo.onButtonPress);
		toast.setButtonLabel(notifInfo.buttonLabel.c_str());

		ImGui::InsertNotification(toast);
	}

	void ImGuiNotifications::RenderNotifications()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
		ImGui::RenderNotifications(); // <-- Here we render all notifications
		ImGui::PopStyleVar(1); // Don't forget to Pop()
		ImGui::PopStyleColor(1);
	}

	ImGuiToastType ImGuiNotifications::ToastTypeFromNotificationType(ImGuiNotificationType type)
	{
		switch (type)
		{
			case ImGuiNotificationType::Info: return ImGuiToastType::Info;
			case ImGuiNotificationType::Warning: return ImGuiToastType::Warning;
			case ImGuiNotificationType::Error: return ImGuiToastType::Error;
			case ImGuiNotificationType::Success: return ImGuiToastType::Success;
			default: return ImGuiToastType::None;
		}
	}
}

