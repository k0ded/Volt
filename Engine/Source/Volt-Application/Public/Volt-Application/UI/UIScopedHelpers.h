#pragma once

#include "Volt-Application/Config.h"
#include "Volt-Application/UI/UITypes.h"

#include <glm/vec4.hpp>

#include <imgui.h>

namespace UI
{
	enum class FontType;

	class ScopedColor
	{
	public:
		ScopedColor(ImGuiCol_ color, const glm::vec4& newColor)
			:m_color(color)
		{
			auto& colors = ImGui::GetStyle().Colors;
			m_oldColor = colors[color];
			colors[color] = newColor;
		}

		~ScopedColor()
		{
			auto& colors = ImGui::GetStyle().Colors;
			colors[m_color] = m_oldColor;
		}

	private:
		ImVec4 m_oldColor;
		ImGuiCol_ m_color;
	};

	class ScopedButtonColor
	{
	public:
		ScopedButtonColor(const ButtonColorInfo& newColors)
		{
			auto& colors = ImGui::GetStyle().Colors;
			m_oldNormalColor = colors[ImGuiCol_Button];
			m_oldHoveredColor = colors[ImGuiCol_ButtonHovered];
			m_oldActiveColor = colors[ImGuiCol_ButtonActive];

			colors[ImGuiCol_Button] = newColors.normal;
			colors[ImGuiCol_ButtonHovered] = newColors.hovered;
			colors[ImGuiCol_ButtonActive] = newColors.active;
		}

		~ScopedButtonColor()
		{
			auto& colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Button] = m_oldNormalColor;
			colors[ImGuiCol_ButtonHovered] = m_oldHoveredColor;
			colors[ImGuiCol_ButtonActive] = m_oldActiveColor;
		}


	private:
		glm::vec4 m_oldNormalColor;
		glm::vec4 m_oldHoveredColor;
		glm::vec4 m_oldActiveColor;
	};

	class ScopedColorPredicate
	{
	public:
		ScopedColorPredicate(bool predicate, ImGuiCol_ color, const glm::vec4& newColor)
			: m_oldColor(0),
			m_color(color), 
			m_predicate(predicate)
		{
			if (!m_predicate)
			{
				return;
			}

			auto& colors = ImGui::GetStyle().Colors;
			m_oldColor = colors[color];
			colors[color] = ImVec4{ newColor.x, newColor.y, newColor.z, newColor.w };
		}

		~ScopedColorPredicate()
		{
			if (!m_predicate)
			{
				return;
			}

			auto& colors = ImGui::GetStyle().Colors;
			colors[m_color] = m_oldColor;
		}

	private:
		glm::vec4 m_oldColor;
		ImGuiCol_ m_color;

		bool m_predicate = false;
	};

	class ScopedStyleFloat
	{
	public:
		ScopedStyleFloat(ImGuiStyleVar_ var, float value)
		{
			ImGui::PushStyleVar(var, value);
		}

		~ScopedStyleFloat()
		{
			ImGui::PopStyleVar();
		}
	};

	class ScopedStyleFloat2
	{
	public:
		ScopedStyleFloat2(ImGuiStyleVar_ var, const glm::vec2& value)
		{
			ImGui::PushStyleVar(var, { value.x, value.y });
		}

		~ScopedStyleFloat2()
		{
			ImGui::PopStyleVar();
		}
	};
}
