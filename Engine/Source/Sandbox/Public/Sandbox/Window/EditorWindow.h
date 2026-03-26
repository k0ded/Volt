#pragma once

#include "Sandbox/EditorCommandStack.h"

#include <EventSystem/EventListener.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace Volt
{
	class Asset;
	class Event;
}

class EditorWindow : public Volt::EventListener
{
public:
	EditorWindow(const String& title, bool dockSpace = false, String id = "");
	virtual ~EditorWindow() = default;

	bool Begin();
	void End();
	void Open();
	void Close();

	void SetMinWindowSize(ImVec2 minSize);

	void Focus();
	bool IsDocked() const;

	virtual void UpdateMainContent() = 0;
	virtual void UpdateContent() {}
	virtual void OpenAsset(AssetReference<Volt::Asset> asset) {}

	inline const String& GetTitle() const { return m_title; }
	inline const bool& IsOpen() const { return m_isOpen; }
	inline const bool IsFocused() const { return m_isFocused; }
	inline const bool IsHovered() const { return m_isHovered; }

	inline EditorCommandStack& GetCommandStack() { return myCommandStack; }

protected:
	virtual void OnClose() {}
	virtual void OnOpen() {}

	void ForceWindowDocked(ImGuiWindow* childWindow);
	inline ImGuiWindowClass* GetWindowClass() { return &m_windowClass; };

	String m_title;
	String m_id;
	EditorCommandStack myCommandStack{};

	ImGuiWindowFlags m_windowFlags = 0;
	ImGuiWindowFlags m_nodePanelFlags = 0;

	ImVec2 m_minSize = { -1.f, -1.f };
	glm::vec4 m_backgroundColor = 0.f;

	bool m_isDockspace = false;
	bool m_hasDockspace = false;

	bool m_isFullscreenImage = false;
	bool m_previousFrameOpen = false;

	ImGuiID m_mainDockID = 0;
	ImGuiWindowClass m_windowClass;

private:
	bool m_isOpen = false;
	bool m_isFocused = false;
	bool m_isHovered = false;

	std::unordered_map<ImGuiID, ImGuiID> m_dockIDs;
};
