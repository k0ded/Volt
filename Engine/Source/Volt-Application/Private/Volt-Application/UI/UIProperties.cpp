#include "vtapppch.h"
#include "Volt-Application/UI/UIProperties.h"
#include "Volt-Application/UI/UIUtility.h"
#include "Volt-Application/UI/UIScopedHelpers.h"

#include <imgui_internal.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/FileSystem.h>

namespace UI
{
	static int32_t s_propertiesContextIndex = -1;
	static Vector<uint32_t> s_propertiesContextStackIDs;

	int32_t GetAndIncrementPropertiesStackID()
	{
		if (s_propertiesContextIndex >= s_propertiesContextStackIDs.size() || s_propertiesContextIndex < 0)
		{
			return 0;
		}
		return s_propertiesContextStackIDs[s_propertiesContextIndex]++;
	}

	std::string MakePropertyID()
	{
		return "##Properties_" + std::to_string(s_propertiesContextIndex) + "_" + std::to_string(GetAndIncrementPropertiesStackID());
	}

	bool BeginProperties(const std::string& name, const glm::vec2 size)
	{
		bool open = ImGui::BeginTable(name.c_str(), 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable, size);

		if (open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);
			ImGui::PushStyleColor(ImGuiCol_Border, { 49.f / 255.f, 49.f / 255.f, 49.f / 255.f, 1.f });

			ImGui::PushStyleColor(ImGuiCol_FrameBg, { 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, { 15.f / 255.f, 15.f / 255.f, 15.f / 255.f, 1.f });

			ImGui::PushStyleColor(ImGuiCol_Separator, { 26.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, { 26.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.f });
			ImGui::PushStyleColor(ImGuiCol_SeparatorActive, { 26.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.f });

			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.3f);
			ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch);

			s_propertiesContextIndex++;
			s_propertiesContextStackIDs.push_back(0);
			VT_ENSURE(s_propertiesContextIndex == s_propertiesContextStackIDs.size() - 1);
		}

		return open;
	}

	void EndProperties()
	{
		ImGui::EndTable();
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar(2);

		s_propertiesContextIndex--;
		s_propertiesContextStackIDs.pop_back();
	}

	void UI::BeginPropertyRow()
	{
		auto* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = PROPERTY_ROW_HEIGHT;

		ImGui::TableNextRow(0, PROPERTY_ROW_HEIGHT);
		ImGui::TableNextColumn();
		window->DC.CurrLineTextBaseOffset = 3.f;

		SetPropertyBackgroundColor();
	}

	void UI::EndPropertyRow()
	{}

	bool Property(const std::string& text, bool& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::Checkbox(id.c_str(), &value);
		});

		EndPropertyRow();

		return changed;
	}

	void PropertyInfoString(const std::string& key, const std::string& info)
	{
		BeginPropertyRow();

		ImGui::TextUnformatted(key.c_str());

		ImGui::TableNextColumn();
		ImGui::TextUnformatted(info.c_str());

		EndPropertyRow();
	}

	bool Property(const std::string& text, int32_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&id, &value]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_S32, (void*)&value, 1.f);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, uint32_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_U32, (void*)&value, 1.f);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, int16_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_S16, (void*)&value, 1.f);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, uint16_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_U16, (void*)&value, 1.f);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, int8_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_S8, (void*)&value, 1.f);
		});

		EndPropertyRow();
		return changed;
	}

	bool Property(const std::string& text, uint8_t& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_U8, (void*)&value, 1.f);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, double& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragScalar(id.c_str(), ImGuiDataType_Double, (void*)&value, 1.f);
		});

		EndPropertyRow();
		return changed;
	}

	bool Property(const std::string& text, float& value, float min, float max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			bool c = false;

			if (min != 0.f && max != 0.f)
			{
				c = ImGui::SliderFloat(id.c_str(), &value, min, max);
			}
			else
			{
				c = ImGui::DragFloat(id.c_str(), &value, 0.1f);
			}

			return c;
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::vec2& value, float min, float max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_Float, glm::value_ptr(value), 2, 0.1f, &min, &max);

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::vec3& value, float min, float max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_Float, glm::value_ptr(value), 3, 0.1f, &min, &max);

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::vec4& value, float min, float max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_Float, glm::value_ptr(value), 4, 0.1f, &min, &max);
		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::uvec2& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_U32, glm::value_ptr(value), 2, 1.f, &min, &max);

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::uvec3& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_U32, glm::value_ptr(value), 3, 1.f, &min, &max);

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::uvec4& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_U32, glm::value_ptr(value), 4, 1.f, &min, &max);
		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::ivec2& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_S32, glm::value_ptr(value), 2, 1.f, &min, &max);

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::ivec3& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_S32, glm::value_ptr(value), 3, 1.f, &min, &max);
		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::ivec4& value, uint32_t min, uint32_t max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DragScalarN(id.c_str(), ImGuiDataType_S32, glm::value_ptr(value), 4, 1.f, &min, &max);
		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, glm::quat& value, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		constexpr float min = -1.f;
		constexpr float max = 1.f;

		changed = DragScalarN(id.c_str(), ImGuiDataType_Float, glm::value_ptr(value), 4, 1.f, &min, &max);
		EndPropertyRow();

		return changed;
	}

	bool PropertyDragFloat(const std::string& text, float& value, float increment, float min, float max, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return ImGui::DragFloat(id.c_str(), &value, increment, min, max);
		});

		EndPropertyRow();

		return changed;
	}

	bool PropertyTextBox(const std::string& text, const std::string& value, bool readOnly, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return InputText("", id, const_cast<std::string&>(value), readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, const std::string& value, bool readOnly, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return InputText("", id, const_cast<std::string&>(value), readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
		});

		EndPropertyRow();

		return changed;
	}

	bool Property(const std::string& text, std::string& value, bool readOnly, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return InputText("", id, value, readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
		});

		EndPropertyRow();
		return changed;
	}

	bool PropertyColor(const std::string& text, glm::vec4& value, const std::string& toolTip)
	{
		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		bool changed = DrawItem([&]()
		{
			return ImGui::ColorEdit4(id.c_str(), glm::value_ptr(value));
		});

		EndPropertyRow();
		return changed;
	}

	bool PropertyColor(const std::string& text, glm::vec3& value, const std::string& toolTip)
	{
		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		bool changed = DrawItem([&]()
		{
			return ImGui::ColorEdit3(id.c_str(), glm::value_ptr(value));
		});

		EndPropertyRow();
		return changed;
	}

	bool Property(const std::string& text, std::filesystem::path& path, const std::filesystem::path& baseDir, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);

		ImGui::TableNextColumn();
		std::string sPath = path.string();
		std::string id = MakePropertyID();

		changed = DrawItem(ImGui::GetColumnWidth() - ImGui::CalcTextSize("Open...").x - 20.f, [&]()
		{
			if (InputText("", id, sPath))
			{
				path = std::filesystem::path(sPath);
				return true;
			}

			return false;
		});

		ImGui::SameLine();

		std::string buttonId = "Open...##" + MakePropertyID();
		if (ImGui::Button(buttonId.c_str(), { ImGui::GetContentRegionAvail().x, 25.f }))
		{
			auto newPath = FileSystem::OpenFileDialogue({ { "All (*.*)" }, { "*" } }, baseDir);
			if (!newPath.empty())
			{
				path = newPath;
				changed = true;
			}
		}

		EndPropertyRow();

		return changed;
	}
	bool PropertyDirectory(const std::string& text, std::filesystem::path& path, const std::filesystem::path& baseDir, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string sPath = path.string();
		std::string id = MakePropertyID();

		changed = DrawItem(ImGui::GetColumnWidth() - ImGui::CalcTextSize("Open...").x - 20.f, [&]()
		{
			if (InputText("", id, sPath))
			{
				path = std::filesystem::path(sPath);
				return true;
			}

			return false;
		});

		ImGui::SameLine();

		std::string buttonId = "Open..." + MakePropertyID();
		if (ImGui::Button(buttonId.c_str(), { ImGui::GetContentRegionAvail().x, 25.f }))
		{
			auto newPath = FileSystem::PickFolderDialogue(baseDir);
			if (!newPath.empty())
			{
				path = newPath;
				changed = true;
			}
		}

		EndPropertyRow();

		return changed;
	}

	bool UI::ComboProperty(const std::string& text, int& currentItem, const Vector<const char*>& items, float width)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		ImGui::TableNextColumn();

		std::string id = MakePropertyID();
		changed = DrawItem((width == 0.f) ? ImGui::GetColumnWidth() : width, [&]()
		{
			return ImGui::Combo(id.c_str(), &currentItem, items.data(), (int32_t)items.size());
		});

		EndPropertyRow();

		return changed;
	}

	bool UI::ComboProperty(const std::string& text, int& currentItem, const Vector<std::string>& strItems, float width)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		ImGui::TableNextColumn();

		std::string id = MakePropertyID();

		Vector<const char*> items;
		std::for_each(strItems.begin(), strItems.end(), [&](const std::string& string) { items.emplace_back(string.c_str()); });

		changed = DrawItem((width == 0.f) ? ImGui::GetColumnWidth() : width, [&]()
		{
			return ImGui::Combo(id.c_str(), &currentItem, items.data(), (int32_t)items.size());
		});

		EndPropertyRow();

		return changed;
	}


	bool PropertyMultiline(const std::string& text, std::string& value, bool readOnly, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return InputTextMultiline("", id, value, readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None);
		});

		EndPropertyRow();

		return changed;
	}

	bool PropertyPassword(const std::string& text, std::string& value, bool readOnly, const std::string& toolTip)
	{
		bool changed = false;

		BeginPropertyRow();

		ImGui::TextUnformatted(text.c_str());
		SimpleToolTip(toolTip);
		ImGui::TableNextColumn();
		std::string id = MakePropertyID();

		changed = DrawItem([&]()
		{
			return InputText("", id, value, readOnly ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_Password);
		});

		EndPropertyRow();
		return changed;
	}


	bool UI::IsPropertyRowHovered()
	{
		const ImVec2 rowAreaMin = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
		const ImVec2 rowAreaMax = { ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), ImGui::TableGetColumnCount() - 1).Max.x, rowAreaMin.y + PROPERTY_ROW_HEIGHT + PROPERTY_ROW_PADDING * 2.f };

		ImGui::PushClipRect(rowAreaMin, rowAreaMax, false);
		const bool isRowHovered = ImGui::IsMouseHoveringRect(rowAreaMin, rowAreaMax, true);
		ImGui::PopClipRect();

		return isRowHovered;
	}

	bool UI::IsPropertyColumnHovered(const uint32_t column)
	{
		const ImVec2 rowAreaMin = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), static_cast<int32_t>(column)).Min;
		const ImVec2 rowAreaMax = { ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), static_cast<int32_t>(column)).Max.x, rowAreaMin.y + PROPERTY_ROW_HEIGHT + PROPERTY_ROW_PADDING * 2.f };

		ImGui::PushClipRect(rowAreaMin, rowAreaMax, false);
		const bool isColumnHovered = ImGui::IsMouseHoveringRect(rowAreaMin, rowAreaMax, true);
		ImGui::PopClipRect();

		return isColumnHovered;
	}

	void UI::SetPropertyBackgroundColor()
	{
		static const glm::vec4 PropertyBackground = { 36.f / 255.f, 36.f / 255.f, 36.f / 255.f, 1.f };
		static const glm::vec4 PropertyBackgroundHovered = { 47.f / 255.f, 47.f / 255.f, 47.f / 255.f, 1.f };

		if (IsPropertyRowHovered())
		{
			SetRowColor(PropertyBackgroundHovered);
		}
		else
		{
			SetRowColor(PropertyBackground);
		}
	}

	bool PropertyAxisColor(const std::string& text, glm::vec3& value, float resetValue)
	{
		ScopedStyleFloat2 cellPad(ImGuiStyleVar_CellPadding, { 4.f, 0.f });

		bool changed = false;

		BeginPropertyRow();

		ImGui::Text(text.c_str());

		ImGui::TableNextColumn();

		const auto width = ImGui::CalcItemWidth() / 3;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.f, 0.f });

		//todo_fabian: verify (font size no longer exists)
		//float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
		float lineHeight = GImGui->Style.FramePadding.y * 2.f;
		ImVec2 buttonSize = { lineHeight + 3.f, lineHeight };

		{
			ScopedColor color{ ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f } };
			ScopedColor colorh{ ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f } };
			ScopedColor colora{ ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f } };

			std::string butId = "X" + MakePropertyID();
			if (ImGui::Button(butId.c_str(), buttonSize))
			{
				value.x = resetValue;
				changed = true;
			}
		}

		ImGui::SameLine();
		std::string id = MakePropertyID();

		changed |= DrawItem(width, [&]()
		{
			return ImGui::DragFloat(id.c_str(), &value.x, 0.1f);
		});

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				changed = true;
			}
		}

		ImGui::SameLine();

		{
			ScopedColor color{ ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f } };
			ScopedColor colorh{ ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f } };
			ScopedColor colora{ ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f } };

			std::string butId = "Y" + MakePropertyID();
			if (ImGui::Button(butId.c_str(), buttonSize))
			{
				value.y = resetValue;
				changed = true;
			}
		}

		ImGui::SameLine();
		id = MakePropertyID();

		changed |= DrawItem(width, [&]()
		{
			return ImGui::DragFloat(id.c_str(), &value.y, 0.1f);
		});

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				changed = true;
			}
		}

		ImGui::SameLine();

		{
			ScopedColor color{ ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f } };
			ScopedColor colorh{ ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f } };
			ScopedColor colora{ ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f } };

			std::string butId = "Z" + MakePropertyID();
			if (ImGui::Button(butId.c_str(), buttonSize))
			{
				value.z = resetValue;
				changed = true;
			}
		}

		ImGui::SameLine();
		id = MakePropertyID();

		changed |= DrawItem(width, [&]()
		{
			return ImGui::DragFloat(id.c_str(), &value.z, 0.1f);
		});

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				changed = true;
			}
		}

		ImGui::PopStyleVar();

		EndPropertyRow();

		return changed;
	}

	bool PropertyAxisColor(const std::string& text, glm::vec2& value, float resetValue)
	{
		ScopedStyleFloat2 cellPad(ImGuiStyleVar_CellPadding, { 4.f, 0.f });

		bool changed = false;

		BeginPropertyRow();

		ImGui::Text(text.c_str());

		ImGui::TableNextColumn();
		const float width = ImGui::CalcItemWidth() / 2;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.f, 0.f });

		//todo_fabian: verify (font size no longer exists)
		//float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
		float lineHeight = GImGui->Style.FramePadding.y * 2.f;
		ImVec2 buttonSize = { lineHeight + 3.f, lineHeight };

		{
			ScopedColor color{ ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f } };
			ScopedColor colorh{ ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f } };
			ScopedColor colora{ ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f } };

			std::string butId = "X" + MakePropertyID();
			if (ImGui::Button(butId.c_str(), buttonSize))
			{
				value.x = resetValue;
				changed = true;
			}
		}

		ImGui::SameLine();
		std::string id = MakePropertyID();

		changed |= DrawItem(width, [&]()
		{
			return ImGui::DragFloat(id.c_str(), &value.x, 0.1f);
		});

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				changed = true;
			}
		}

		ImGui::SameLine();

		{
			ScopedColor color{ ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f } };
			ScopedColor colorh{ ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f } };
			ScopedColor colora{ ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f } };

			std::string butId = "Y" + MakePropertyID();
			if (ImGui::Button(butId.c_str(), buttonSize))
			{
				value.y = resetValue;
				changed = true;
			}
		}

		ImGui::SameLine();
		id = MakePropertyID();

		changed |= DrawItem(width, [&]()
		{
			return ImGui::DragFloat(id.c_str(), &value.y, 0.1f);
		});

		if (ImGui::IsItemHovered())
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				changed = true;
			}
		}

		ImGui::PopStyleVar();

		EndPropertyRow();

		return changed;
	}
}
