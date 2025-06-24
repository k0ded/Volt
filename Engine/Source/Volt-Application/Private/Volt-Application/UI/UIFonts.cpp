#include "vtapppch.h"
#include "Volt-Application/UI/UIFonts.h"
#include <CoreUtilities/Containers/Map.h>



namespace UI
{
	static Map<FontType, ImFont*> s_fonts;

	void SetFont(FontType type, ImFont* font)
	{
		s_fonts[type] = font;
	}

	ImFont* GetFont(FontType fontType)
	{
		if (!s_fonts.contains(fontType))
		{
			return nullptr;
		}
		return s_fonts[fontType];
	}

	void PushFont(FontType font)
	{
		ImGui::PushFont(GetFont(font));
	}

	void PopFont()
	{
		ImGui::PopFont();
	}

}
