#pragma once

#include "Mosaic/Config.h"

#include <CoreUtilities/Containers/Vector.h>

#include <CoreUtilities/String/VoltString.h>
#include <CoreUtilities/String/StringBuilder.h>

namespace Mosaic
{
	class VTMOSAIC_API MosaicShaderWriter
	{
	public:
		struct TextureDeclaration
		{
			uint32_t index;
			String name;
		};

		void AppendCodeBlock(const String& codeBlockStr);
		String AddTexture(uint32_t textureIndex);

		String GetAsString() const;

		VT_NODISCARD VT_INLINE const Vector<TextureDeclaration>& GetTextureDeclarations() const { return m_textureDeclarations; }

	private:
		StringBuilder m_shaderCode;
		
		Vector<TextureDeclaration> m_textureDeclarations;
	};
}
