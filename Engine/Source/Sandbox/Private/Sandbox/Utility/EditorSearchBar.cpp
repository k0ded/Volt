#include "sbpch.h"
#include "Sandbox/Utility/EditorSearchBar.h"
#include "Sandbox/Utility/EditorResources.h"

#include <Volt-Application/UI/UIScopedHelpers.h>
#include <Volt-Application/UI/UIUtility.h>

EditorSearchBar::EditorSearchBar(const glm::vec4& backgroundColor, bool renderWithinChildWindow)
	: m_backgroundColor(backgroundColor), m_renderWithinChildWindow(renderWithinChildWindow)
{

}

bool EditorSearchBar::Render(float width, bool setAsFocused)
{
	UI::ScopedColor childColor{ ImGuiCol_ChildBg, m_backgroundColor };
	UI::ScopedStyleFloat rounding(ImGuiStyleVar_ChildRounding, 2.f);

	constexpr float BarHeight = 32.f;
	constexpr float SearchBarIconSize = 22.f;

	const bool hasSpecifiedWidth = width > 0.f;
	const float childWidth = hasSpecifiedWidth ? width : ImGui::GetContentRegionAvail().x;

	bool returnValue = false;
	bool shouldRender = true;

	if (m_renderWithinChildWindow)
	{
		shouldRender = ImGui::BeginChild("##searchBar", { childWidth, BarHeight });
	}

	if (shouldRender)
	{
		UI::ShiftCursor(5.f, 4.f);
		ImGui::Image(UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Search)), { SearchBarIconSize, SearchBarIconSize });

		ImGui::SameLine();

		const float itemWidth = hasSpecifiedWidth ? width : ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x;
		ImGui::PushItemWidth(itemWidth);

		UI::PushID();

		if (setAsFocused)
		{
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.f);

		returnValue = UI::InputTextWithHint("", m_searchQuery, "Search...");
		
		UI::PopID();

		ImGui::PopStyleVar();
		ImGui::PopItemWidth();
	}

	if (m_renderWithinChildWindow)
	{
		ImGui::EndChild();
	}

	return returnValue;
}
