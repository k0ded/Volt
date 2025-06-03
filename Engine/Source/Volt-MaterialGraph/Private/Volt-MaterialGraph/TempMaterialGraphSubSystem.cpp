#include "vtmgpch.h"
#include "TempMaterialGraphSubSystem.h"

#include <RHIModule/ImGui/ImGuiImplementation.h>

#include <imgui.h>

namespace Volt
{
	VT_REGISTER_SUBSYSTEM(TempMaterialGraphSubSystem, Default, PostEngine, 0);

	void TempMaterialGraphSubSystem::Initialize()
	{
		//ImGui::SetCurrentContext(RHI::ImGuiImplementation::Get().GetContext());
	}
}
