#include "vtassetspch.h"

#include "Volt-Assets/SourceAssetImporters/TextureImportCommon.h"

#include <CoreUtilities/StringUtility.h>


namespace Volt::TextureImport
{
	TextureCompressionType TryGetTextureCompressionTypeFromFilename(const ::std::string& filename)
	{
		::std::string lowerFilename = Utility::ToLower(filename);

		// It's a base color texture
		if (lowerFilename.contains("_basecolor") || 
			lowerFilename.contains("_bc") ||
			lowerFilename.contains("_diff") ||
			lowerFilename.contains("_base_color"))
		{
			return TextureCompressionType::BC1;
		}

		// Normal map
		if (lowerFilename.contains("_n") ||
			lowerFilename.contains("_normal") ||
			lowerFilename.contains("_ddna") ||
			lowerFilename.contains("_ddn"))
		{
			return TextureCompressionType::BC5;
		}

		// Metallic roughness/mask
		if (lowerFilename.contains("_specular") ||
			lowerFilename.contains("_mr") ||
			lowerFilename.contains("_mre"))
		{
			return TextureCompressionType::BC3;
		}

		// Emissive
		if (lowerFilename.contains("_emissive"))
		{
			return TextureCompressionType::BC1;
		}

		// Default to BC5
		return TextureCompressionType::BC1;
	}
}
