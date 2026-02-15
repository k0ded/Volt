#pragma once

#include <RenderCore/RenderGraph/RenderGraph.h>

class TestingRenderGraph : public Volt::RenderGraph
{
public:
	VT_INLINE const Volt::RGVector<Volt::RGPassRef>& GetPasses() const { return m_renderPasses; }
	VT_INLINE const Volt::RGVector<Volt::RGCompiledPass>& GetCompiledPasses() const { return m_compiledRenderPasses; }
	VT_INLINE const uint32_t GetNumCompiledPasses() const { return static_cast<uint32_t>(m_compiledRenderPasses.size()); }
};

template<typename ParameterStruct>
inline void AddComputePass(Volt::RenderGraph& renderGraph, Volt::RenderGraphPassFlags flags, const ParameterStruct* passParameters)
{
	renderGraph.AddPass("Compute Pass",
		Volt::RenderGraphPassFlags::Compute | flags,
		passParameters,
		[](Volt::RenderContext& context)
	{});
}

template<typename ParameterStruct>
inline void AddRasterPass(Volt::RenderGraph& renderGraph, Volt::RenderGraphPassFlags flags, const ParameterStruct* passParameters)
{
	renderGraph.AddPass("Raster Pass",
		flags,
		passParameters,
		[](Volt::RenderContext& context)
	{});
}

BEGIN_SHADER_PARAMETER_STRUCT(WriteSingleBufferParameters)
	SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWBuffer)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(ReadSingleBufferParameters)
	SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, Buffer)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(WriteSingleTextureParameters)
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D<float4>, RWTexture)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(ReadSingleTextureParameters)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Texture)
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(RenderTargetParameters)
	RG_RENDER_TARGETS()
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(RenderTargetWithSingleTextureReadParameters)
	SHADER_PARAMETER_TEXTURE_SRV(Texture2D<float4>, Texture)
	RG_RENDER_TARGETS()
END_SHADER_PARAMETER_STRUCT()
