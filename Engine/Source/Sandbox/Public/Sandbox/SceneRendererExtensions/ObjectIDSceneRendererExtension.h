#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>
#include <Volt-Renderer/GPUScene.h>

#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/RenderGraph/ShaderParameterStruct.h>
#include <RenderCore/Shader/GlobalShader.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt::RHI
{
	class Image;
}

struct ObjectIDVS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(ObjectIDVS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
		SHADER_PARAMETER_STRUCT_INCLUDE(Volt::GPUSceneParameters, GPUScene)
	END_SHADER_PARAMETER_STRUCT()
};

struct ObjectIDPS : public Volt::GlobalShader
{
	DECLARE_GLOBAL_SHADER(ObjectIDPS)
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		RG_RENDER_TARGETS()
	END_SHADER_PARAMETER_STRUCT()
};

struct ObjectIDTexture
{
	Volt::RGTextureRef texture;
};

class ObjectIDSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	ObjectIDSceneRendererExtension(Ref<Volt::RenderScene> renderScene)
		: Volt::SceneRendererExtension(renderScene)
	{}

	~ObjectIDSceneRendererExtension() override = default;

	Volt::RGTextureRef OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, const Volt::RenderView& view, Volt::RGTextureRef prevOutputImage) override;
	void OnRegistered(Volt::MeshPassProcessorRegistry& meshPassProcessorRegistry) override;

	VT_INLINE VT_NODISCARD RefPtr<Volt::RHI::Image> GetIDImage() const { return m_objectIdImage; }

private:
	RefPtr<Volt::RHI::Image> m_objectIdImage;

	class ObjectIDPassMeshProcessor* m_meshPassProcessor = nullptr;
};
