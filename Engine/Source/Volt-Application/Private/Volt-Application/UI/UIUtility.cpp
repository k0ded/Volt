#include "vtapppch.h"
#include "Volt-Application/UI/UIUtility.h"
#include "Volt-Application/UI/UIScopedHelpers.h"
#include "Volt-Application/UI/UIFonts.h"
#include "Volt-Application/UI/ImGuiSubSystem.h"

#include <Volt-Renderer/Texture/Texture2D.h>

#include <SubSystem/SubSystemManager.h>
#include <SubSystem/SubSystem.h>
#include <EventSystem/ApplicationEvents.h>

#include <CoreUtilities/StringUtility.h>

#include <InputModule/Input.h>

inline static glm::vec4 ToNormalizedRGB(float r, float g, float b, float a = 255.f)
{
	return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
}

namespace UI
{
	static uint32_t s_contextId = 0;
	static uint32_t s_stackId = 0;

	class ResetStackEventListener : public SubSystem, Volt::EventListener
	{
	public:
		ResetStackEventListener()
		{
			RegisterListener<Volt::AppUpdateEvent>([](Volt::AppUpdateEvent& e) 
			{
				s_stackId = 0;

				return false;
			});
		}

		VT_DECLARE_SUBSYSTEM("{549B4945-CFF4-4DC3-9B9E-7F495425EBED}"_guid);
	};
	VT_REGISTER_SUBSYSTEM(ResetStackEventListener, Minimal, PreEngine, 0);

	ImTextureID GetTextureID(Ref<Volt::Texture2D> texture, int32_t mipIndex)
	{
		Volt::ImGuiSubSystem* subSystem = SubSystemManager::GetSubSystem<Volt::ImGuiSubSystem>();
		return subSystem->GetTextureID(texture->GetImage(), mipIndex);

		//return Volt::RHI::ImGuiImplementation::Get().GetTextureID(texture->GetImage(), mipIndex);
	}

	ImTextureID GetTextureID(RefPtr<Volt::RHI::Image> texture, int32_t mipIndex)
	{
		Volt::ImGuiSubSystem* subSystem = SubSystemManager::GetSubSystem<Volt::ImGuiSubSystem>();
		return subSystem->GetTextureID(texture, mipIndex);
		//return Volt::RHI::ImGuiImplementation::Get().GetTextureID(texture, mipIndex);
	}

	void Header(const std::string& text)
	{
		ScopedFont font{ UI::FontType::Regular, UI::BIG_FONT_SIZE };
		ImGui::TextUnformatted(text.c_str());
	}

	Volt::ImGuiNotificationType ImGuiNotificationTypeFromNotificationType(NotificationType type)
	{
		switch (type)
		{
			case UI::NotificationType::Info: return Volt::ImGuiNotificationType::Info;
			case UI::NotificationType::Warning: return Volt::ImGuiNotificationType::Warning;
			case UI::NotificationType::Error: return Volt::ImGuiNotificationType::Error;
			case UI::NotificationType::Success: return Volt::ImGuiNotificationType::Success;
			default: return Volt::ImGuiNotificationType::None;
		}
	}

	void ShiftCursor(float x, float y)
	{
		ImVec2 pos = { ImGui::GetCursorPosX() + x, ImGui::GetCursorPosY() + y };
		ImGui::SetCursorPos(pos);
	}

	bool BeginPopup(const std::string& name, ImGuiWindowFlags flags)
	{
		flags |= ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings;

		const uint32_t nameHash = static_cast<uint32_t>(std::hash<std::string>()(name));
		return ImGui::BeginPopupEx(nameHash, flags);
	}

	bool BeginPopupItem(const std::string& id, ImGuiPopupFlags flags)
	{
		if (id.empty())
		{
			return ImGui::BeginPopupContextItem();
		}

		return ImGui::BeginPopupContextItem(id.c_str(), flags);
	}

	bool BeginPopupWindow(const std::string& id)
	{
		if (id.empty())
		{
			return ImGui::BeginPopupContextWindow();
		}

		return ImGui::BeginPopupContextWindow(id.c_str());
	}

	void EndPopup()
	{
		ImGui::EndPopup();
	}

	int32_t LevenshteinDistance(const std::string& str1, const std::string& str2)
	{
		int32_t m = (int32_t)str1.length();
		int32_t n = (int32_t)str2.length();
		Vector<Vector<int>> dp(m + 1, Vector<int>(n + 1));

		// Initializing the first row and column as 0
		for (int i = 0; i <= m; i++)
		{
			dp[i][0] = i;
		}
		for (int j = 0; j <= n; j++)
		{
			dp[0][j] = j;
		}

		// Filling in the rest of the dp array
		for (int i = 1; i <= m; i++)
		{
			for (int j = 1; j <= n; j++)
			{
				int insertion = dp[i][j - 1] + 1;
				int deletion = dp[i - 1][j] + 1;
				int match = dp[i - 1][j - 1];
				int mismatch = dp[i - 1][j - 1] + 1;
				if (str1[i - 1] == str2[j - 1])
				{
					dp[i][j] = std::min(std::min(insertion, deletion), match);
				}
				else
				{
					dp[i][j] = std::min(std::min(insertion, deletion), mismatch);
				}
			}
		}
		return dp[m][n];
	}

	const Vector<std::string> GetEntriesMatchingQuery(const std::string& query, const Vector<std::string>& entries)
	{
		std::multimap<int32_t, std::string> scores{};

		for (const auto& entry : entries)
		{
			const int32_t score = LevenshteinDistance(query, entry);
			scores.emplace(score, entry);
		}

		Vector<std::string> result{};
		for (const auto& [score, entry] : scores)
		{
			if (!Utility::StringContains(Utility::ToLower(entry), Utility::ToLower(query)))
			{
				continue;
			}

			result.emplace_back(entry);
		}

		return result;
	}

	void RenderMatchingTextBackground(const std::string& query, const std::string& text, const glm::vec4& color, const glm::uvec2& offset)
	{
		const auto matchOffset = Utility::ToLower(text).find(Utility::ToLower(query));

		if (matchOffset == std::string::npos)
		{
			return;
		}

		const auto matchPrefix = text.substr(0, matchOffset);
		const auto match = text.substr(matchOffset, query.size());

		const auto prefixSize = ImGui::CalcTextSize(matchPrefix.c_str());
		const auto matchSize = ImGui::CalcTextSize(match.c_str());
		const auto cursorPos = ImGui::GetCursorPos();
		const auto windowPos = ImGui::GetWindowPos();
		const auto scrollX = ImGui::GetScrollX();
		const auto scrollY = ImGui::GetScrollY();

		auto currentWindow = ImGui::GetCurrentWindow();
		const ImColor imguiCol = ImColor(color.x, color.y, color.z, color.w);

		const ImVec2 min = { cursorPos.x - scrollX + prefixSize.x + offset.x, cursorPos.y - scrollY + offset.y };
		const ImVec2 max = { cursorPos.x - scrollX + prefixSize.x + matchSize.x + offset.x, cursorPos.y + matchSize.y - scrollY + offset.y };

		currentWindow->DrawList->AddRectFilled(min + windowPos, max + windowPos, imguiCol);
	}

	void RenderHighlightedBackground(const glm::vec4& color, float height)
	{
		auto currentWindow = ImGui::GetCurrentWindow();
		const auto windowPos = ImGui::GetWindowPos();
		const auto availRegion = ImGui::GetContentRegionMax();
		const auto cursorPos = ImGui::GetCursorPos();

		const auto scrollX = ImGui::GetScrollX();
		const auto scrollY = ImGui::GetScrollY();

		const ImVec2 min = ImGui::GetWindowPos() + ImVec2{ -scrollX, cursorPos.y - scrollY };
		const ImVec2 max = ImGui::GetWindowPos() + ImVec2{ availRegion.x - scrollX, height + cursorPos.y - scrollY };
		currentWindow->DrawList->AddRectFilled(min, max, ImColor{ color.x, color.y, color.z, color.w });
	}


	void SetRowColor(const glm::vec4& color)
	{
		for (int32_t i = 0; i < ImGui::TableGetColumnCount(); i++)
		{
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor{ color.x, color.y, color.z, color.w }, i);
		}
	}


	bool IsItemHovered(const float itemWidth)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();

		const auto& style = ImGui::GetStyle();

		const ImVec2 label_size = ImGui::CalcTextSize("TEST", NULL, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(itemWidth, label_size.y + style.FramePadding.y * 2.0f));

		if (ImGui::IsMouseHoveringRect(frame_bb.Min, frame_bb.Max))
		{
			return true;
		}

		return false;
	}

	bool DragScalarN(const std::string& id, ImGuiDataType dataType, void* data, int32_t components, float speed, const void* min, const void* max)
	{
		static constexpr ImGuiDataTypeInfo GDataTypeInfo[] =
		{
			{ sizeof(char),             "S8",   "%d",   "%d"    },  // ImGuiDataType_S8
			{ sizeof(unsigned char),    "U8",   "%u",   "%u"    },
			{ sizeof(short),            "S16",  "%d",   "%d"    },  // ImGuiDataType_S16
			{ sizeof(unsigned short),   "U16",  "%u",   "%u"    },
			{ sizeof(int),              "S32",  "%d",   "%d"    },  // ImGuiDataType_S32
			{ sizeof(unsigned int),     "U32",  "%u",   "%u"    },
		#ifdef _MSC_VER
			{ sizeof(ImS64),            "S64",  "%I64d","%I64d" },  // ImGuiDataType_S64
			{ sizeof(ImU64),            "U64",  "%I64u","%I64u" },
		#else
			{ sizeof(ImS64),            "S64",  "%lld", "%lld"  },  // ImGuiDataType_S64
			{ sizeof(ImU64),            "U64",  "%llu", "%llu"  },
		#endif
			{ sizeof(float),            "float", "%.3f","%f"    },  // ImGuiDataType_Float (float are promoted to double in va_arg)
			{ sizeof(double),           "double","%f",  "%lf"   },  // ImGuiDataType_Double
		};

		bool changed = false;

		ImGui::BeginGroup();
		ImGui::PushID(id.c_str());
		const float width = (ImGui::GetColumnWidth() / components) - ImGui::GetStyle().ItemInnerSpacing.x;
		size_t type_size = GDataTypeInfo[dataType].Size;

		for (int32_t i = 0; i < components; i++)
		{
			ImGui::PushID(i);

			if (i > 0)
			{
				ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			}

			changed |= DrawItem(width, [&]()
			{
				return ImGui::DragScalar("", dataType, data, speed, min, max);
			});

			ImGui::PopID();
			data = (void*)((char*)data + type_size);
		}

		ImGui::PopID();

		ImGui::EndGroup();
		return changed;
	}

	bool DrawItem(std::function<bool()> itemFunc)
	{
		const float itemWidth = ImGui::GetColumnWidth();

		ImGui::PushItemWidth(itemWidth);

		const bool itemHovered = IsItemHovered(itemWidth);

		if (itemHovered)
		{
			static const glm::vec4 PropertyItemHovered = { 1.f };
			ImGui::PushStyleColor(ImGuiCol_Border, PropertyItemHovered);
		}

		bool changed = itemFunc();

		if (itemHovered)
		{
			ImGui::PopStyleColor();
		}

		ImGui::PopItemWidth();

		return changed;
	}

	bool DrawItem(const float itemWidth, std::function<bool()> itemFunc)
	{
		ImGui::PushItemWidth(itemWidth);

		const bool itemHovered = IsItemHovered(itemWidth);

		if (itemHovered)
		{
			static const glm::vec4 PropertyItemHovered = { 1.f };
			ImGui::PushStyleColor(ImGuiCol_Border, PropertyItemHovered);
		}

		bool changed = itemFunc();

		if (itemHovered)
		{
			ImGui::PopStyleColor();
		}

		ImGui::PopItemWidth();

		return changed;
	}

	bool InputTextWithHint(const std::string& name, std::string& text, const std::string& hint, ImGuiInputTextFlags_ flags /* = ImGuiInputTextFlags_None */)
	{
		std::string id = "##" + std::to_string(GetAndIncrementStackID());
		return InputTextWithHint(name, id, text, hint, flags);
	}

	bool InputTextMultiline(const std::string& name, std::string& text, ImGuiInputTextFlags_ flags)
	{
		std::string id = "##" + std::to_string(GetAndIncrementStackID());
		return InputTextMultiline(name, text, id, flags);
	}

	bool InputText(const std::string& name, std::string& text, ImGuiInputTextFlags_ flags)
	{
		std::string id = "##" + std::to_string(GetAndIncrementStackID());
		return InputText(name, id, text, flags);
	}

	bool InputText(const std::string& name, const std::string& id, std::string& text, ImGuiInputTextFlags_ flags)
	{
		if (!name.empty())
		{
			ImGui::TextUnformatted(name.c_str());
			ImGui::SameLine();
		}

		return ImGui::InputTextString(id.c_str(), &text, flags);
	}

	bool InputTextWithHint(const std::string& name, const std::string& id, std::string& text, const std::string& hint, ImGuiInputTextFlags_ flags)
	{
		if (!name.empty())
		{
			ImGui::TextUnformatted(name.c_str());
			ImGui::SameLine();
		}

		return ImGui::InputTextWithHintString(id.c_str(), hint.c_str(), &text, flags);
	}

	bool InputTextMultiline(const std::string& name, const std::string& id, std::string& text, ImGuiInputTextFlags_ flags)
	{
		if (!name.empty())
		{
			ImGui::TextUnformatted(name.c_str());
			ImGui::SameLine();
		}

		return ImGui::InputTextMultilineString(id.c_str(), &text, ImVec2{ 0.f, 0.f }, flags);
	}

	bool ImageButton(const std::string& id, ImTextureID textureId, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		const ImGuiID imId = window->GetID(id.c_str());

		return ImGui::ImageButtonEx(imId, textureId, size, uv0, uv1, bg_col, tint_col);
	}

	bool ImageButton(const std::string& id, ImTextureID textureId, const ImVec2& size, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		const ImGuiID imId = window->GetID(id.c_str());

		const ImVec2& uv0 = ImVec2(0, 0);
		const ImVec2& uv1 = ImVec2(1, 1);

		return ImGui::ImageButtonEx(imId, textureId, size, uv0, uv1, bg_col, tint_col);
	}

	bool ImageButtonState(const std::string& id, bool state, ImTextureID textureId, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1)
	{
		if (state)
		{
			return ImageButton(id, textureId, size, uv0, uv1, { 0.18f, 0.18f, 0.18f, 1.f });
		}
		else
		{
			return ImageButton(id, textureId, size, uv0, uv1);
		}
	}

	bool TreeNodeImage(Ref<Volt::Texture2D> texture, const std::string& text, ImGuiTreeNodeFlags flags, bool setOpen)
	{
		ScopedStyleFloat2 frame{ ImGuiStyleVar_FramePadding, { 0.f, 0.f } };
		ScopedStyleFloat2 spacing{ ImGuiStyleVar_ItemSpacing, { 0.f, 0.f } };

		const ImVec2 size = ImGui::CalcTextSize(text.c_str());

		ImGui::Image(GetTextureID(texture), { size.y, size.y });
		ImGui::SameLine();

		if (setOpen)
		{
			ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		}

		return ImGui::TreeNodeEx(text.c_str(), flags);
	}

	bool TreeNodeFramed(const std::string& text, bool alwaysOpen, float rounding)
	{
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;

		if (alwaysOpen)
		{
			nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		ScopedStyleFloat frameRound(ImGuiStyleVar_FrameRounding, rounding);

		return ImGui::TreeNodeEx(text.c_str(), nodeFlags);
	}

	void SameLine(float offsetX, float spacing)
	{
		ImGui::SameLine(offsetX, spacing);
	}

	bool ImageSelectable(Ref<Volt::Texture2D> texture, const std::string& text, bool& selected)
	{
		const ImVec2 size = ImGui::CalcTextSize(text.c_str());

		ImGui::Image(GetTextureID(texture), { size.y, size.y });
		ImGui::SameLine();

		return ImGui::Selectable(text.c_str(), &selected);
	}

	bool ImageSelectable(Ref<Volt::Texture2D> texture, const std::string& text)
	{
		const ImVec2 size = ImGui::CalcTextSize(text.c_str());

		ImGui::Image(GetTextureID(texture), { size.y - 2.f, size.y - 2.f });
		ImGui::SameLine();

		ShiftCursor(5.f, 0.f);

		return ImGui::Selectable(text.c_str());
	}

	bool ImageSelectable(Ref<Volt::Texture2D> texture, const std::string& text, bool selected)
	{
		ImVec2 size = ImGui::CalcTextSize(text.c_str());
		ImGui::Image(GetTextureID(texture), { size.y, size.y }, { 0, 1 }, { 1, 0 });
		ImGui::SameLine();
		return ImGui::Selectable(text.c_str(), selected, ImGuiSelectableFlags_SpanAvailWidth);
	}

	void PushID()
	{
		int id = s_contextId++;
		ImGui::PushID(id);
	}

	void PopID()
	{
		ImGui::PopID();
		s_contextId--;
	}

	int32_t GetAndIncrementStackID()
	{
		int32_t newId = 0;
		if (s_stackId != UINT32_MAX) [[likely]]
		{
			newId = s_stackId++;
		}
		else
		{
			s_stackId = 0;
		}
		return newId;
	}

	bool IsInputEnabled()
	{
		const auto& io = ImGui::GetIO();
		return (io.ConfigFlags & ImGuiConfigFlags_NoMouse) == 0 && (io.ConfigFlags & ImGuiConfigFlags_NavNoCaptureKeyboard) == 0;
	}

	void SetInputEnabled(bool enable)
	{
		auto& io = ImGui::GetIO();

		if (enable)
		{
			io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			io.ConfigFlags &= ~ImGuiConfigFlags_NavNoCaptureKeyboard;
		}
		else
		{
			io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
			io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
		}
	}

	void SimpleToolTip(const std::string& toolTip)
	{
		if (!toolTip.empty())
		{
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", toolTip.c_str());
			}
		}
	}

	bool BeginMenuBar(ImRect barRect)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		barRect.Min.y += padding.y;
		barRect.Max.y += padding.y;

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
		ImRect bar_rect = barRect;
		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x)), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
			IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize) + window->Pos.x)), IM_ROUND(bar_rect.Max.y + window->Pos.y));
		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analoguous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;
		ImGui::AlignTextToFramePadding();
		return true;
	}

	void EndMenuBar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
				const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavCursorVisible = true; // Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavHighlightItemUnderNav = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		// IM_ASSERT(window->Flags & ImGuiWindowFlags_MenuBar); // NOTE(Yan): Needs to be commented out because Jay
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

	bool BeginListView(const std::string& strId)
	{
		const glm::vec4 BACKGROUND = ToNormalizedRGB(26.f, 26.f, 26.f);

		ImGui::PushStyleColor(ImGuiCol_ChildBg, BACKGROUND);

		bool open = ImGui::BeginChild(strId.c_str(), ImGui::GetContentRegionAvail());
		return open;
	}

	void EndListView()
	{
		ImGui::PopStyleColor();
	}


	bool Combo(const std::string& text, int& currentItem, const Vector<const char*>& items, float width)
	{
		bool changed = false;

		ImGui::TextUnformatted(text.c_str());

		ImGui::SameLine();

		std::string id = "##" + std::to_string(GetAndIncrementStackID());

		ImGui::SetNextItemWidth(width);
		if (ImGui::Combo(id.c_str(), &currentItem, items.data(), (int32_t)items.size()))
		{
			changed = true;
		}

		return changed;
	}

	bool Combo(const std::string& text, int& currentItem, const Vector<std::string>& strItems, float width)
	{
		bool changed = false;

		ImGui::TextUnformatted(text.c_str());

		ImGui::SameLine();

		std::string id = "##" + std::to_string(GetAndIncrementStackID());

		Vector<const char*> items;
		std::for_each(strItems.begin(), strItems.end(), [&](const std::string& string) { items.emplace_back(string.c_str()); });

		ImGui::SetNextItemWidth(width);
		if (ImGui::Combo(id.c_str(), &currentItem, items.data(), (int32_t)items.size()))
		{
			changed = true;
		}

		return changed;
	}

	void Notify(NotificationType type, const std::string& title, const std::string& content, int32_t duration)
	{
		Volt::ImGuiNotificationInfo info;
		info.type = ImGuiNotificationTypeFromNotificationType(type);
		info.dismissTime = duration;
		info.title = title.c_str();
		info.message = content.c_str();

		Volt::ImGuiNotifications::InsertNotification(info);
	}

	void OpenModal(const std::string& name, ImGuiPopupFlags flags)
	{
		const uint32_t nameHash = static_cast<uint32_t>(std::hash<std::string>()(name));
		ImGui::OpenPopupEx(nameHash, flags);
	}

	void OpenPopup(const std::string& name, ImGuiPopupFlags flags)
	{
		const uint32_t nameHash = static_cast<uint32_t>(std::hash<std::string>()(name));
		ImGui::OpenPopupEx(nameHash, flags);
	}

	bool BeginModal(const std::string& name, ImGuiWindowFlags flags)
	{
		const uint32_t nameHash = static_cast<uint32_t>(std::hash<std::string>()(name));
		return ImGui::BeginPopupModal(name.c_str(), nameHash, nullptr, flags);
	}

	void EndModal()
	{
		ImGui::EndPopup();
	}

	void SmallSeparatorHeader(const std::string& text, float padding)
	{
		ScopedFont font{ UI::FontType::Bold, UI::DEFAULT_FONT_SIZE };

		const auto pos = ImGui::GetCursorPos();
		ImGui::TextUnformatted(text.c_str());
		const auto textSize = ImGui::CalcTextSize(text.c_str());

		const auto availWidth = ImGui::GetWindowWidth();
		const auto windowPos = ImGui::GetWindowPos();

		ImGui::GetCurrentWindow()->DrawList->AddLine(pos + windowPos + ImVec2{ textSize.x + padding, textSize.y / 2.f }, { pos.x + availWidth + windowPos.x, pos.y + 1.f + windowPos.y + textSize.y / 2.f }, IM_COL32(255, 255, 255, 255));
	}

	bool Combo(const std::string& text, int& currentItem, const char** items, uint32_t count)
	{
		bool changed = false;

		ImGui::TextUnformatted(text.c_str());

		ImGui::SameLine();

		std::string id = "##" + std::to_string(GetAndIncrementStackID());

		if (ImGui::Combo(id.c_str(), &currentItem, items, count))
		{
			changed = true;
		}

		return changed;
	}



	void TreeNodePop()
	{
		ImGui::TreePop();
	}

	bool CollapsingHeader(std::string_view label, ImGuiTreeNodeFlags flags)
	{
		return ImGui::CollapsingHeader(label.data(), flags);
	}

	


}
