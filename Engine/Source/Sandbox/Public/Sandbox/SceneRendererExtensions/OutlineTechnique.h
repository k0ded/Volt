#pragma once

#include "Volt-Renderer/GPUScene.h"

#include <EntitySystem/EntityID.h>

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/Shader/GlobalShader.h>

#include <unordered_set>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;
	struct DrawCullingData;
	struct RenderView;
}

struct OutlineGeometryVS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(OutlineGeometryVS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
		SHADER_PARAMETER_BUFFER_SRV(StructuredBuffer<uint>, PrimitivesToDraw)
		SHADER_PARAMETER_STRUCT_INCLUDE(Volt::GPUSceneParameters, GPUScene)
	END_SHADER_PARAMETER_STRUCT()
};

struct OutlineGeometryPS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(OutlineGeometryPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};

class OutlineMeshPassProcessor;

struct OutlineTechnique
{
	OutlineTechnique(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, OutlineMeshPassProcessor* meshPassProcessor);

	void Execute(Volt::RGTextureRef dstImage, Volt::RenderScene& renderScene, const Volt::RenderView& view, const std::unordered_set<Volt::EntityID>& selectedEntities);

private:
	Volt::RGTextureRef AddDrawOutlineGeometryPass(Volt::RenderScene& renderScene, const Volt::RenderView& view, const std::unordered_set<Volt::EntityID>& selectedEntities);
	Volt::RGTextureRef AddJumpFloodInitPass(Volt::RGTextureRef outlineGeometryImage, const Volt::RenderView& view);
	Volt::RGTextureRef AddJumpFloodPass(Volt::RGTextureRef prevImage, const Volt::RenderView& view, int32_t step);
	void AddOutlineCompositePass(Volt::RGTextureRef dstImage, const Volt::RenderView& view, Volt::RGTextureRef jumpfloodOutput);

	Volt::RenderGraph& m_renderGraph;
	Volt::RenderGraphBlackboard& m_blackboard;

	OutlineMeshPassProcessor* m_meshPassProcessor;
};
