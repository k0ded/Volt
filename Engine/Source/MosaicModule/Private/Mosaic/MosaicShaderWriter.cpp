#include "mcpch.h"

#include "Mosaic/MosaicShaderWriter.h"

namespace Mosaic
{
	void MosaicShaderWriter::AppendCodeBlock(const std::string& codeBlockStr)
	{
		m_shaderCode << codeBlockStr;
	}

	std::string MosaicShaderWriter::AddTexture(uint32_t textureIndex)
	{
		auto& newTexture = m_textureDeclarations.emplace_back();
		newTexture.index = textureIndex;
		newTexture.name = std::format("Texture_{}", textureIndex);

		return newTexture.name;
	}

	std::string MosaicShaderWriter::GetAsString() const
	{
		return m_shaderCode.str();
	}
}
