#pragma once

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

		const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : g.Style.FramePadding;
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
}
