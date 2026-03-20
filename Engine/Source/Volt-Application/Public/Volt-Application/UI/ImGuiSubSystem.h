#pragma once

#include "Volt-Application/Config.h"

#include <Volt-ImGui/ImGuiImplementation.h>
#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <EventSystem/EventListener.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <LogModule/LogCategory.h>

#include <imgui.h>

VT_DECLARE_LOG_CATEGORY(LogImGuiSubSystem, LogVerbosity::Trace);

struct ImGuiContext;
namespace Volt
{
	class ImGuiImplementation;
	class AppBeginFrameEvent;
	class AppPresentFrameEvent;

	class VTAPP_API ImGuiSubSystem : public SubSystem, public EventListener
	{
	public:
		ImGuiSubSystem();

		void Initialize() override;
		void Shutdown() override;

		void InitializeImGui(bool enableViewports = true);

		void Begin();
		void End();

		void EnterBlockingContext(std::function<void()> onEnterCallback = {});
		void ExitBlockingContext();

		ImTextureID GetTextureID(IntRef<RHI::Image> image, int32_t mipIndex = -1);

		VT_NODISCARD VT_INLINE bool IsInitialized() const { return m_imguiImplementation != nullptr; }
		VT_NODISCARD VT_INLINE bool IsWithinImGuiUpdate() const { return m_isWithinImGuiUpdate; }

		VT_DECLARE_SUBSYSTEM("{482BA05C-2FFA-4457-9FFD-7B14833C8212}"_guid);

	private:
		bool OnAppBeginFrameEvent(AppBeginFrameEvent& e);
		bool OnAppPresentFrameEvent(AppPresentFrameEvent& e);

		bool m_isBlockingActive = false;
		bool m_isWithinFrame = false;
		bool m_isWithinImGuiUpdate = false;
		Ref<ImGuiImplementation> m_imguiImplementation;
	};
}
