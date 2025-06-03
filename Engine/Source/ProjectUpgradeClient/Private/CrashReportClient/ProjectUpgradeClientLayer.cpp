#include "ProjectUpgradeClient/ProjectUpgradeClientLayer.h"

#include <EventSystem/ApplicationEvents.h>

#include <Volt-Application/BaseApplication.h>
#include <Volt-Application/UI/UIUtility.h>
#include <Volt-Application/UI/ImGuiSubSystem.h>

#include <SubSystem/SubSystemManager.h>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace Volt
{
	void ProjectUpgradeClientLayer::OnAttach()
	{
		RegisterListener<Volt::AppUpdateEvent>(VT_BIND_EVENT_FN(ProjectUpgradeClientLayer::OnUpdateEvent));
		RegisterListener<Volt::AppImGuiUpdateEvent>(VT_BIND_EVENT_FN(ProjectUpgradeClientLayer::OnImGuiUpdateEvent));
	}

	void ProjectUpgradeClientLayer::OnDetach()
	{
	}

	bool ProjectUpgradeClientLayer::OnUpdateEvent(Volt::AppUpdateEvent& e)
	{
		return false;
	}

	bool ProjectUpgradeClientLayer::OnImGuiUpdateEvent(Volt::AppImGuiUpdateEvent& e)
	{
		auto& io = ImGui::GetIO();

		ImGui::SetNextWindowPos({ 0, 0 });
		ImGui::SetNextWindowSize({ io.DisplaySize.x, io.DisplaySize.y });

		if (ImGui::Begin("Window", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::Text("The project you tried to edit is out of date!");
			ImGui::End();
		}

		return false;
	}
}
