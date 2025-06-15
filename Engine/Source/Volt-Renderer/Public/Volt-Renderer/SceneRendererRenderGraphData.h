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
		
		RGTextureRef sceneColor;
	};

	struct EnvironmentTextures
	{
		RGTextureRef irradiance;
		RGTextureRef radiance;
		RGTextureRef DFGLuT;
	};

	struct DefaultTextures
	{
		RGTextureRef black1x1Cube;
		RGTextureRef white1x1;
	};

	struct LightScene
	{
		RGBufferRef visibleLightIndices;
	};
}
