#include "sbpch.h"
#include "Sandbox/Window/AssetBrowser/BrowserItems.h"

#include "Sandbox/Window/AssetBrowser/AssetBrowserSelectionManager.h"

#include "Sandbox/Utility/AssetBrowserUtilities.h"
#include "Sandbox/UserSettingsManager.h"

#include <Volt/Utility/UIUtility.h>
#include <InputModule/InputCodes.h>

namespace AssetBrowser
{
	Item::Item(SelectionManager* selectionManager, const std::filesystem::path& aPath)
		: m_selectionManager(selectionManager), path(aPath)
	{
		m_isRenaming = false;
		m_lastRenaming = false;
	}
	bool Item::Render()
	{
		if (m_typeName.empty())
		{
			m_typeName = GetTypeName();
		}

		bool reload = false;

		PushID();
		const float thumbnailSize = GetThumbnailSize();
		const RefPtr<Volt::RHI::Image> icon = GetIcon();
		const bool isSelected = m_selectionManager->IsSelected(this);

		const ImVec2 itemSize = AssetBrowserUtilities::GetBrowserItemSize(thumbnailSize);
		const float itemPadding = AssetBrowserUtilities::GetBrowserItemPadding();
		const float itemHeightModifier = AssetBrowserUtilities::GetItemHeightModifier();
		const ImVec4 assetTypeColor = GetBackgroundColor();

		UI::ScopedColor outerChild{ ImGuiCol_ChildBg, { 0.f } };
		ImGui::BeginChild("hoverWindow", itemSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		{
			const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
			const ImVec4 bgColor = AssetBrowserUtilities::GetBackgroundColor(hovered, isSelected);

			UI::ScopedColor middleChild{ ImGuiCol_ChildBg, { bgColor.x, bgColor.y, bgColor.z, bgColor.w } };
			UI::ScopedStyleFloat rounding{ ImGuiStyleVar_ChildRounding, 2.f };
			ImGui::BeginChild("item", itemSize, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			{
				UI::ShiftCursor(itemPadding / 2.f, itemPadding / 2.f);

				constexpr float colorModifier = 0.6f;
				UI::ScopedColor innerChild{ ImGuiCol_ChildBg, { assetTypeColor.x * colorModifier, assetTypeColor.y * colorModifier, assetTypeColor.z * colorModifier, assetTypeColor.w } };
				ImGui::BeginChild("Image", { thumbnailSize, thumbnailSize + 4.f }, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
				{
					// Icon
					{
						ImGui::Image(UI::GetTextureID(icon), { GetThumbnailSize(), thumbnailSize });

						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
						{
							SetDragDropPayload();
							ImGui::TextUnformatted("Move:");

							constexpr uint32_t maxShownPaths = 3;
							for (uint32_t i = 0; const auto & selected : m_selectionManager->GetSelectedItems())
							{
								if (i == maxShownPaths)
								{
									ImGui::TextUnformatted("...");
									break;
								}
								ImGui::TextUnformatted(selected->path.string().c_str());
								i++;
							}

							ImGui::EndDragDropSource();
						}
					}

					const ImVec2 maxChildPos = AssetBrowserUtilities::GetBrowserItemPos();

					// Bottom bar
					{
						const ImVec2 barMin = maxChildPos;
						const ImVec2 barMax = barMin + ImVec2{ 10.f, 4.f };

						ImVec4 color = assetTypeColor;
						color.w = 1.f;

						ImGui::GetWindowDrawList()->AddRectFilled(barMin, barMax, ImColor(assetTypeColor));
					}
				}

				ImGui::EndChild();


				auto cursorPos = ImGui::GetCursorPos();
				//Type name color at the bottom of the item
				{
					UI::ScopedColor typeNameColor(ImGuiCol_Text, GetTypeNameColor(hovered, isSelected));
					UI::ScopedFont typeFont(FontType::Regular_12);
					UI::ShiftCursor(itemPadding / 2.f, itemHeightModifier - ImGui::CalcTextSize(m_typeName.c_str()).y - itemPadding * 2.f);
					ImGui::TextUnformatted(m_typeName.c_str());
				}

				ImGui::SetCursorPos(cursorPos);
				UI::ShiftCursor(itemPadding / 2.f, 0.f);

				if (m_isRenaming)
				{
					const std::string renameId = "###renameId" + path.stem().string();
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

					UI::ScopedColor background{ ImGuiCol_FrameBg, { 0.1f, 0.1f, 0.1f, 0.1f } };
					if (ImGui::InputTextString(renameId.c_str(), &m_currentRenamingName, ImGuiInputTextFlags_EnterReturnsTrue))
					{
						if (Rename(m_currentRenamingName))
						{
							m_isRenaming = false;
							reload = true;
							ImGui::PopItemWidth();
							goto renderEnd;
						}
					}

					if (m_isRenaming != m_lastRenaming)
					{
						const ImGuiID widgetId = ImGui::GetCurrentWindow()->GetID(renameId.c_str());
						ImGui::SetFocusID(widgetId, ImGui::GetCurrentWindow());
						ImGui::SetKeyboardFocusHere(-1);
					}

					if (!ImGui::IsItemFocused())
					{
						if (Rename(m_currentRenamingName))
						{
							m_isRenaming = false;
							reload = true;
							ImGui::PopItemWidth();
							goto renderEnd;
						}
					}

					if (!ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					{
						if (Rename(m_currentRenamingName))
						{
							m_isRenaming = false;
							reload = true;
							ImGui::PopItemWidth();
							goto renderEnd;
						}
					}

					m_lastRenaming = true;
					ImGui::PopItemWidth();
				}
				else
				{
					m_lastRenaming = false;
					ImGui::TextWrapped("%s", path.stem().string().c_str());
				}


				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered)
				{
					Open();
				}

				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && hovered && !m_selectionManager->IsSelected(this))
				{
					m_selectionManager->DeselectAll();
					m_selectionManager->Select(this);
				}

				const bool mouseDown = m_selectionManager->IsAnySelected() ? ImGui::IsMouseReleased(ImGuiMouseButton_Left) : ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				if (mouseDown && hovered)
				{
					if (!Volt::Input::IsKeyDown(Volt::InputCode::LeftControl))
					{
						m_selectionManager->DeselectAll();
					}

					if (m_selectionManager->IsSelected(this))
					{
						m_selectionManager->Deselect(this);
					}
					else
					{
						m_selectionManager->Select(this);
					}
				}

			}

		renderEnd:
			ImGui::EndChild();
		}
		auto pos = ImGui::GetWindowPos();
		ImGui::EndChild();
		ImGui::PopID();

		const auto popupID = ("RightClickAssetBrowserItemPopup" + path.string());
		bool tileHovered = ImGui::IsMouseHoveringRect({ pos.x, pos.y }, { pos.x + itemSize.x, pos.y + itemSize.y }) &&
			ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		if (tileHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup(popupID.c_str());
		}
		const bool rightClickMenuOpen = ImGui::IsPopupOpen(popupID.c_str());
		if (rightClickMenuOpen)
		{
			if (ImGui::BeginPopup(popupID.c_str(), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
			{
				if (RenderRightClickPopup())
				{
					reload = true;
				}
				ImGui::EndPopup();
			}
		}

		if (tileHovered && !rightClickMenuOpen)
		{
			ImGui::BeginTooltip();
			UI::PushFont(FontType::Regular_20);
			ImGui::TextEx(path.stem().string().c_str(), nullptr);
			UI::PopFont();

			ImGui::SameLine();

			UI::PushFont(FontType::Regular_16);
			ImGui::Text(("(" + m_typeName + ")").c_str());
			UI::PopFont();

			ImGui::Separator();
			auto pathNoName = path;
			pathNoName._Remove_filename_and_separator();
			DrawHoverInfo("Path", pathNoName.string());
			
			DrawAdditionalHoverInfo();
			ImGui::EndTooltip();
		}

		return reload;
	}
	void Item::StartRename()
	{
		m_isRenaming = true;
		m_currentRenamingName = path.stem().string();
	}
	float Item::GetThumbnailSize() const
	{
		return UserSettingsManager::GetSettings().assetBrowserSettings.thumbnailSize;
	}
	void Item::DrawHoverInfo(std::string_view aInfoTitle, std::string_view aInfo)
	{
		const ImVec4 infoTitleColor = { 0.6f,0.6f,0.6f,1 };
		ImGui::PushStyleColor(ImGuiCol_Text, infoTitleColor);
		ImGui::TextUnformatted((std::string(aInfoTitle) + ": ").c_str());
		ImGui::PopStyleColor();

		ImGui::SameLine();

		ImGui::TextUnformatted(aInfo.data());
	}
	glm::vec4 Item::GetTypeNameColor(bool aHoverFlag, bool aSelectedFlag) const
	{
		if (aHoverFlag)
		{
			return { 0.8f ,0.8f ,0.8f ,1.f };
		}
		else if (aSelectedFlag)
		{
			return { 0.8f ,0.8f ,0.8f ,1.f };
		}
		return { 0.6f, 0.6f, 0.6f, 1.0f };
	}
}
