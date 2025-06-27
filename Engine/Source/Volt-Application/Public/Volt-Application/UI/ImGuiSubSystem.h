#pragma once

#include "Volt-Application/Config.h"

#include <SubSystem/SubSystem.h>

#include <RHIModule/ImGui/ImGuiImplementation.h>

#include <LogModule/LogCategory.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <imgui.h>


VT_DECLARE_LOG_CATEGORY(LogImGuiSubSystem, LogVerbosity::Trace);

struct ImGuiContext;
namespace Volt
{
	class VTAPP_API ImGuiSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
		void Shutdown() override;

		void InitializeImGui(bool enableViewports = true);

		void Begin();
		void End();

		VT_NODISCARD VT_INLINE bool IsInitialized() const { return m_imguiImplementation != nullptr; }

		VT_DECLARE_SUBSYSTEM("{482BA05C-2FFA-4457-9FFD-7B14833C8212}"_guid);

	private:

		RefPtr<RHI::ImGuiImplementation> m_imguiImplementation;
	};
}
