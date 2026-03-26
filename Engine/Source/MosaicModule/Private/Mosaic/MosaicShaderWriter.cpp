#include "mcpch.h"

#include "Mosaic/MosaicShaderWriter.h"

#include <CoreUtilities/String/StringFormat.h>

namespace Mosaic
{
	void MosaicShaderWriter::AppendCodeBlock(const String& codeBlockStr)
	{
		m_shaderCode << codeBlockStr;
	}

	String MosaicShaderWriter::AddTexture(uint32_t textureIndex)
	{
		auto& newTexture = m_textureDeclarations.emplace_back();
		newTexture.index = textureIndex;
		newTexture.name = FormatString("Texture_{}", textureIndex);

		return newTexture.name;
	}

	String MosaicShaderWriter::GetAsString() const
	{
		return m_shaderCode.Get();
	}
}
