#pragma once

#include <glm/glm.hpp>

#include <string>

class EditorSearchBar
{
public:
	EditorSearchBar(const glm::vec4& backgroundColor, bool renderWithinChildWindow = true);

	bool Render(float width, bool setAsFocused = false);
	
	VT_INLINE const String& GetSearchQuery() const { return m_searchQuery; }

private:
	glm::vec4 m_backgroundColor;
	String m_searchQuery;
	bool m_renderWithinChildWindow;
};
