#include "RHIModule/Core/Core.h"

#include <functional>

enum class ImGuiToastType : uint8_t;



namespace Volt::RHI
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
		VTRHI_API static void InsertNotification(const ImGuiNotificationInfo& notifInfo);
		
		static void RenderNotifications();
	private:
		static ImGuiToastType ToastTypeFromNotificationType(ImGuiNotificationType type);
	};
}
