#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/Shader/GlobalShader.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"

namespace Volt
{
	struct VTRC_API CopyToSwapchain_SDR : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CopyToSwapchain_SDR)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, SrcColor)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};

	struct VTRC_API CopyToSwapchain_HDR : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CopyToSwapchain_SDR)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float3>, SrcColor)
			RG_RENDER_TARGETS()
		END_SHADER_PARAMETER_STRUCT()
	};
}
