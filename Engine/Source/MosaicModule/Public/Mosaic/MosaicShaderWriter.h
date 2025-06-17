#pragma once

#include "Mosaic/Config.h"

#include <CoreUtilities/Containers/Vector.h>

#include <sstream>

namespace Mosaic
{
	class VTMOSAIC_API MosaicShaderWriter
	{
	public:
		struct TextureDeclaration
		{
			uint32_t index;
			std::string name;
		};

		void AppendCodeBlock(const std::string& codeBlockStr);
		std::string AddTexture(uint32_t textureIndex);

		std::string GetAsString() const;

		VT_NODISCARD VT_INLINE const Vector<TextureDeclaration>& GetTextureDeclarations() const { return m_textureDeclarations; }

	private:
		std::stringstream m_shaderCode;
		
		Vector<TextureDeclaration> m_textureDeclarations;
	};
}
