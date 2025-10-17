#include "sbpch.h"
#include "Modals/Modal.h"

#include "Sandbox/Utility/Theme.h"

#include <SubSystem/SubSystemManager.h>

#include <Volt-Application/UI/UIScopedHelpers.h>
#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Application/UI/ImGuiSubSystem.h>

Modal::Modal(const std::string& strId, ImGuiWindowFlags flags)
	: m_strId(strId), m_flags(flags)
{

}

void Modal::Open()
{
	UI::OpenModal(m_strId);
	m_wasOpenLastFrame = false;

	OnOpen();
}

void Modal::OpenBlocking()
{
	RegisterListener<Volt::AppImGuiBlockingUpdateEvent>(VT_BIND_EVENT_FN(Modal::OnImGuiUpdateBlocking));
	m_isBlocking = true;

	Volt::ImGuiSubSystem* imguiSubSystem = SubSystemManager::GetSubSystem<Volt::ImGuiSubSystem>();
	imguiSubSystem->EnterBlockingContext([&]()
	{
		UI::OpenModal(m_strId);
		m_wasOpenLastFrame = false;

		OnOpen();
	});
}

void Modal::Close()
{
	ImGui::CloseCurrentPopup();
	OnClose();

	if (m_isBlocking)
	{
		Volt::ImGuiSubSystem* imguiSubSystem = SubSystemManager::GetSubSystem<Volt::ImGuiSubSystem>();
		imguiSubSystem->ExitBlockingContext();

		UnregisterListener<Volt::AppImGuiBlockingUpdateEvent>();
	}

	m_isBlocking = false;
}

bool Modal::Update()
{
	if (!m_wasOpenLastFrame)
	{
		//const auto viewport = ImGui::GetMainViewport();
		//const auto windowSize = ImGui::GetWindowSize();

		//const ImVec2 targetPos = viewport->GetCenter() - ImVec2{ windowSize.x / 2.f, windowSize.y / 2.f };
		//ImGui::SetNextWindowPos(targetPos);

		m_wasOpenLastFrame = true;
	}

	// Default modal style
	UI::ScopedColor backgroundColorChild{ ImGuiCol_PopupBg, EditorTheme::ToNormalizedRGB(26.f, 26.f, 26.f) };
	UI::ScopedColor frameColor{ ImGuiCol_Header, EditorTheme::ToNormalizedRGB(47.f, 47.f, 47.f) };
	UI::ScopedColor frameColorActive{ ImGuiCol_FrameBgActive, EditorTheme::ToNormalizedRGB(47.f, 47.f, 47.f) };
	UI::ScopedColor frameColorHovered{ ImGuiCol_FrameBgHovered, EditorTheme::ToNormalizedRGB(47.f, 47.f, 47.f) };

	// Default button style
	UI::ScopedButtonColor defaultButtonColor{ EditorTheme::Buttons::DefaultButton };

	const bool modalOpen = UI::BeginModal(m_strId, m_flags);

	if (modalOpen)
	{
		DrawModalContent();
		UI::EndModal();
	}

	return modalOpen;
}

bool Modal::OnImGuiUpdateBlocking(Volt::AppImGuiBlockingUpdateEvent& e)
{
	Update();

	return true;
}
