#pragma once

#include <SubSystem/SubSystem.h>

#include <RHIModule/ImGui/ImGuiImplementation.h>

VT_DECLARE_LOG_CATEGORY(LogImGuiSubSystem, LogVerbosity::Trace);

namespace Volt
{
	class ImGuiSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
		void Shutdown() override;

		void InitializeImGui(bool enableViewports = true);

		void SetupContext();

		void Begin();
		void End();

		VT_NODISCARD VT_INLINE bool IsInitialized() const { return m_imguiImplementation != nullptr; }

		VT_DECLARE_SUBSYSTEM("{482BA05C-2FFA-4457-9FFD-7B14833C8212}"_guid);
	
	private:
		RefPtr<RHI::ImGuiImplementation> m_imguiImplementation;
	};
}
