#pragma once

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

namespace Volt
{
	struct SceneTextures
	{
		RGTextureRef sceneDepth;
		RGTextureRef sceneVelocity;
		RGTextureRef gBufferAlbedo;
		RGTextureRef gBufferNormals;
		RGTextureRef gBufferMaterial;
	};
}
