#pragma once

#include <glm/glm.hpp>

#include <string>

class EditorSearchBar
{
public:
	EditorSearchBar(const glm::vec4& backgroundColor, bool renderWithinChildWindow = true);

	bool Render(float width, bool setAsFocused = false);
	
	VT_INLINE const std::string& GetSearchQuery() const { return m_searchQuery; }

private:
	glm::vec4 m_backgroundColor;
	std::string m_searchQuery;
	bool m_renderWithinChildWindow;
};
