#pragma once

#include <SubSystem/SubSystem.h>

#include <RHIModule/ImGui/ImGuiImplementation.h>

namespace Volt
{
	class ImGuiSubSystem : public SubSystem
	{
	public:
		void Initialize() override;
		void Shutdown() override;

		void SetupContext();

		void Begin();
		void End();

		VT_DECLARE_SUBSYSTEM("{482BA05C-2FFA-4457-9FFD-7B14833C8212}"_guid);
	
	private:
		RefPtr<RHI::ImGuiImplementation> m_imguiImplementation;
	};
}
