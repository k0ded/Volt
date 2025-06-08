#include "rcpch.h"
#include "RenderCore/Shader/DefaultShaders.h"
#include "RenderCore/RenderGraph/ShaderRegistry.h"

namespace Volt
{
#if 0
	REGISTER_SHADER(OpaqueDefaultMaterialCS)
#endif
	REGISTER_SHADER(FullscreenTriangleVS, "Engine/Shaders/Source/Utility/FullscreenTriangle.hlsl", "MainVS", Vertex);
}
