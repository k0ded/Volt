#pragma once
#include "Volt-Application/Config.h"

#include <imgui.h>

namespace UI
{
	enum class FontType
	{
		Regular_12,
		Regular_16,
		Regular_17,
		Regular_20,
		Bold_12,
		Bold_16,
		Bold_17,
		Bold_20,
		Bold_90
	};

	VTAPP_API void SetFont(FontType type, ImFont* font);
	VTAPP_API ImFont* GetFont(FontType);

	VTAPP_API void PushFont(FontType font);
	VTAPP_API void PopFont();

	class VTAPP_API ScopedFont
	{
	public:
		inline ScopedFont(FontType font)
		{
			PushFont(font);
		}

		inline ~ScopedFont()
		{
			PopFont();
		}

	private:
	};

}
