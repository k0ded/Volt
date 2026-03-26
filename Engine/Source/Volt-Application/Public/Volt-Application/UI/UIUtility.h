#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/UI/ImGuiExtension.h"
#include "Volt-Application/UI/UIScopedHelpers.h"
#include "Volt-Application/UI/UIProperties.h"
#include "Volt-Application/UI/UIFonts.h"

#include <glm/glm.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <Volt-Imgui/ImGuiNotifications.h>

#include <CoreUtilities/Pointers/IntRef.h>
#include <CoreUtilities/Pointers/Ref.h>

#include <filesystem>

namespace Volt::RHI
{
	class Texture2D;
	class Image;
}

namespace Volt
{
	class Texture2D;
}

namespace UI
{
	enum class NotificationType
	{
		Info,
		Warning,
		Error,
		Success
	};


	 VTAPP_API ImTextureID GetTextureID(IntRef<Volt::RHI::Image> texture, int32_t mipIndex = -1);
	 VTAPP_API ImTextureID GetTextureID(Ref<Volt::Texture2D> texture, int32_t mipIndex = -1);
	
	 VTAPP_API void Header(const String& text);
	 
	 VTAPP_API Volt::ImGuiNotificationType ImGuiNotificationTypeFromNotificationType(NotificationType type);
	 
	 VTAPP_API void ShiftCursor(float x, float y);
	 
	 VTAPP_API bool BeginPopup(const String& name, ImGuiWindowFlags flags = 0);
	 VTAPP_API bool BeginPopupItem(const String& id = "", ImGuiPopupFlags flags = 0);
	 VTAPP_API bool BeginPopupWindow(const String& id = "");
	 VTAPP_API void EndPopup();
	 
	 VTAPP_API bool InputText(const String& name, String& text, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);
	 VTAPP_API bool InputTextWithHint(const String& name, String& text, const String& hint, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);
	 VTAPP_API bool InputTextMultiline(const String& name, String& text, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);

	 VTAPP_API bool InputText(const String& name, const String& id, String& text, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);
	 VTAPP_API bool InputTextWithHint(const String& name, const String& id, String& text, const String& hint, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);
	 VTAPP_API bool InputTextMultiline(const String& name, const String& id, String& text, ImGuiInputTextFlags_ flags = ImGuiInputTextFlags_None);
	 
	 VTAPP_API bool ImageButton(const String& id, ImTextureID textureId, const ImVec2& size, const ImVec4& bg_col, const ImVec4& tint_col);
	 VTAPP_API bool ImageButton(const String& id, ImTextureID textureId, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& bg_col = ImVec4(0, 0, 0, 0), const ImVec4& tint_col = ImVec4(1, 1, 1, 1));
	 VTAPP_API bool ImageButtonState(const String& id, bool state, ImTextureID textureId, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1));
	
	 VTAPP_API bool TreeNodeImage(IntRef<Volt::RHI::Image> texture, const String& text, ImGuiTreeNodeFlags flags, bool setOpen = false);
	 VTAPP_API bool TreeNodeFramed(const String& text, bool alwaysOpen = false, float rounding = 0.f);
	 VTAPP_API void TreeNodePop();
	
	 VTAPP_API bool CollapsingHeader(StringView label, ImGuiTreeNodeFlags flags = 0);
	 
	 VTAPP_API void SameLine(float offsetX = 0.f, float spacing = -1.f);
	 
	 VTAPP_API bool ImageSelectable(Ref<Volt::Texture2D> texture, const String& text, bool& selected);
	 VTAPP_API bool ImageSelectable(Ref<Volt::Texture2D> texture, const String& text);
	 VTAPP_API bool ImageSelectable(Ref<Volt::Texture2D> texture, const String& text, bool selected);
	
	 VTAPP_API void PushID();
	 VTAPP_API void PopID();
	 VTAPP_API int32_t GetAndIncrementStackID();
	
	 VTAPP_API bool IsInputEnabled();
	 VTAPP_API void SetInputEnabled(bool enable);
	 
	 VTAPP_API void SimpleToolTip(const String& toolTip);
	
	 VTAPP_API bool BeginMenuBar(ImRect barRect);
	 VTAPP_API void EndMenuBar();
	 
	 VTAPP_API bool BeginListView(const String& strId);
	 VTAPP_API void EndListView();
	 
	 VTAPP_API bool Combo(const String& text, int& currentItem, const Vector<const char*>& items, float width = 100.f);
	 VTAPP_API bool Combo(const String& text, int& currentItem, const char** items, uint32_t count);
	 VTAPP_API bool Combo(const String& text, int& currentItem, const Vector<String>& strItems, float width = 100.f);
	 
	 VTAPP_API void Notify(NotificationType type, const String& title, const String& content, int32_t duration = 5000);
	 
	 VTAPP_API void OpenModal(const String& name, ImGuiPopupFlags flags = 0);
	 VTAPP_API void OpenPopup(const String& name, ImGuiPopupFlags flags = 0);
	 
	 VTAPP_API bool BeginModal(const String& name, ImGuiWindowFlags flags = 0);
	 VTAPP_API void EndModal();
	 
	 VTAPP_API void SmallSeparatorHeader(const String& text, float padding);
	 
	 VTAPP_API int32_t LevenshteinDistance(const String& str1, const String& str2);
	 VTAPP_API const Vector<String> GetEntriesMatchingQuery(const String& query, const Vector<String>& entries);
	 
	 VTAPP_API void RenderMatchingTextBackground(const String& query, const String& text, const glm::vec4& color, const glm::uvec2& offset = 0u);
	 VTAPP_API void RenderHighlightedBackground(const glm::vec4& color, float height);
	 
	 VTAPP_API bool DrawItem(std::function<bool()> itemFunc);
	 VTAPP_API bool DrawItem(const float itemWidth, std::function<bool()> itemFunc);

	 //was private
	 VTAPP_API void SetRowColor(const glm::vec4& color);
	 VTAPP_API bool IsItemHovered(const float itemWidth);
	 VTAPP_API bool DragScalarN(const String& id, ImGuiDataType dataType, void* data, int32_t components, float speed, const void* min, const void* max);

	//inline  uint32_t s_contextId = 0;

	 template<typename T> bool DragDropTarget(const String& type, T& outValue)
	 {
		 if (ImGui::BeginDragDropTarget())
		 {
			 if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(type.c_str()))
			 {
				 outValue = *(reinterpret_cast<T*>(pPayload->Data));
				 return true;
			 }

			 ImGui::EndDragDropTarget();
		 }
		 
		 return false;
	 }

	 template<typename T> bool DragDropTarget(std::initializer_list<String> types, T& outValue, ImGuiDragDropFlags flags = 0)
	 {
		 for (const auto& type : types)
		 {
			 if (ImGui::BeginDragDropTarget())
			 {
				 if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload(type.c_str(), flags))
				 {
					 outValue = *(reinterpret_cast<T*>(pPayload->Data));
					 return true;
				 }

				 ImGui::EndDragDropTarget();
			 }
		 }

		 return false;
	 }
};
