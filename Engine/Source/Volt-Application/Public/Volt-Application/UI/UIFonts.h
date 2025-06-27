#pragma once
#include "Volt-Application/Config.h"

#include <imgui.h>

namespace UI
{
	static constexpr float SMALL_FONT_SIZE = 12.f;
	static constexpr float DEFAULT_FONT_SIZE = 16.f;
	static constexpr float BIG_FONT_SIZE = 20.f;
	enum class FontType
	{
		None,
		Regular,
		Bold,
	};

	//Pushing a None font will keep the current font and just push a new size
	VTAPP_API void SetFont(FontType type, ImFont* font);
	VTAPP_API ImFont* GetFont(FontType);

	VTAPP_API void PushFont(FontType font, float fontSize = 0.f);
	VTAPP_API void PopFont();

	class VTAPP_API ScopedFont
	{
	public:
		inline ScopedFont(FontType font, float fontSize = 0.f)
		{
			PushFont(font, fontSize);
		}

		inline ~ScopedFont()
		{
			PopFont();
		}

	private:
	};

}
