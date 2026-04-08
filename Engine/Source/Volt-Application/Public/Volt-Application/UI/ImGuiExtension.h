#pragma once

#include "Volt-Application/Config.h"

#include <CoreUtilities/String/VoltString.h>

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGui
{
	inline static bool ImageButtonAnimated(ImTextureID user_texture_id, ImTextureID texId, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1))
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return false;

		// Default to using texture ID as ID. User can still push string/integer prefixes.
		PushID((void*)(intptr_t)user_texture_id);
		const ImGuiID id = window->GetID("#image");
		PopID();

		return ImageButtonEx(id, texId, size, uv0, uv1, bg_col, tint_col);
	}

	// Horizontal/vertical separating line
	inline static void SeparatorWidthEx(float width, ImGuiSeparatorFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

		float thickness_draw = 1.0f;
		float thickness_layout = 0.0f;
		if (flags & ImGuiSeparatorFlags_Vertical)
		{
			// Vertical separator, for menu bars (use current line height). Not exposed because it is misleading and it doesn't have an effect on regular layout.
			float y1 = window->DC.CursorPos.y;
			float y2 = window->DC.CursorPos.y + window->DC.CurrLineSize.y;
			const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + thickness_draw, y2));
			ItemSize(ImVec2(thickness_layout, 0.0f));
			if (!ItemAdd(bb, 0))
				return;

			// Draw
			window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
			if (g.LogEnabled)
				LogText(" |");
		}
		else if (flags & ImGuiSeparatorFlags_Horizontal)
		{
			// Horizontal Separator
			float x1 = window->DC.CursorPos.x;
			float x2 = window->DC.CursorPos.x + width;

			// FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
			//if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
			//	x1 += window->DC.Indent.x;

			ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
			if (columns)
				PushColumnsBackground();

			// We don't provide our width to the layout so that it doesn't get feed back into AutoFit
			const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
			ItemSize(ImVec2(0.0f, thickness_layout));
			const bool item_visible = ItemAdd(bb, 0);
			if (item_visible)
			{
				// Draw
				window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator));
				if (g.LogEnabled)
					LogRenderedText(&bb.Min, "--------------------------------");
			}
			if (columns)
			{
				PopColumnsBackground();
				columns->LineMinY = window->DC.CursorPos.y;
			}
		}
	}

	inline static void SeparatorWidth(float width)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (window->SkipItems)
			return;

		// Those flags should eventually be overridable by the user
		ImGuiSeparatorFlags flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
		flags |= ImGuiSeparatorFlags_SpanAllColumns;
		SeparatorWidthEx(width, flags);
	}

	// If 'p_open' is specified for a modal popup window, the popup will have a regular close button which will close the popup.
	// Note that popup visibility status is owned by Dear ImGui (and manipulated with e.g. OpenPopup).
	// - *p_open set back to false in BeginPopupModal() when popup is not open.
	// - if you set *p_open to false before calling BeginPopupModal(), it will close the popup.
	inline static bool BeginPopupModal(const char* name, ImGuiID id, bool* p_open, ImGuiWindowFlags flags)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		if (!IsPopupOpen(id, ImGuiPopupFlags_None))
		{
			g.NextWindowData.ClearFlags(); // We behave like Begin() and need to consume those values
			if (p_open && *p_open)
				*p_open = false;
			return false;
		}

		// Center modal windows by default for increased visibility
		// (this won't really last as settings will kick in, and is mostly for backward compatibility. user may do the same themselves)
		// FIXME: Should test for (PosCond & window->SetWindowPosAllowFlags) with the upcoming window.
		if ((g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasPos) == 0)
		{
			const ImGuiViewport* viewport = window->WasActive ? window->Viewport : GetMainViewport(); // FIXME-VIEWPORT: What may be our reference viewport?
			SetNextWindowPos(viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		}

		flags |= ImGuiWindowFlags_Popup | ImGuiWindowFlags_Modal | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
		const bool is_open = Begin(name, p_open, flags);
		if (!is_open || (p_open && !*p_open)) // NB: is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
		{
			EndPopup();
			if (is_open)
				ClosePopupToLevel(g.BeginPopupStack.Size, true);
			return false;
		}
		return is_open;
	}

	VTAPP_API bool InputText(const char* label, String* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	VTAPP_API bool InputTextMultiline(const char* label, String* str, const ImVec2& size = ImVec2(0, 0), ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
	VTAPP_API bool InputTextWithHint(const char* label, const char* hint, String* str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = NULL, void* user_data = NULL);
}
