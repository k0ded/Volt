#include "vrpch.h"

#include "Volt-Renderer/Texture/EnvironmentTexture.h"

#include <AssetSystem/AssetFactory.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::EnvironmentTexture, EnvironmentTexture);

	EnvironmentTexture::EnvironmentTexture(RefPtr<RHI::Image> diffuseImage, RefPtr<RHI::Image> specularImage)
		: m_diffuseImage(diffuseImage), m_specularImage(specularImage)
	{

	}
}
