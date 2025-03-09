#include "vrpch.h"
#include "Volt-Renderer/RenderingTechniques/DDGI.h"
#include "Volt-Renderer/SceneRendererStructs.h"
#include "Volt-Renderer/GPUScene.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RayTracing/RayTracingScene.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphBlackboard.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/RenderGraph/ShaderRegistryMacros.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	static ConsoleVariable<float> s_ddgiProbeSpacing("r.DDGI.ProbeSpacing", 100.f, "How far probes are placed from each other");
	static ConsoleVariable<int32_t> s_ddgiProbeGridSize("r.DDGI.GridSize", 30, "Size of the probe grid");
	static ConsoleVariable<int32_t> s_ddgiProbeResolution("r.DDGI.ProbeResolution", 8, "Resolution of probe in atlas");

	struct DDGIUpdateProbesCS
	{
		BEGIN_SHADER_DEFINITION(DDGIUpdateProbesCS)
			DECLARE_SHADER_STAGE("Engine/Shaders/Source/DDGI/DDGIUpdateProbes.hlsl", "main", RHI::ShaderStage::Compute)
		END_SHADER_DEFINITION()

		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(vt::UniformBuffer<ViewData>, View)
			SHADER_PARAMETER_IMAGE(vt::RWTex2D<float3>, RWProbeIrradianceAtlas)
			SHADER_PARAMETER(float, ProbeSpacing)
			SHADER_PARAMETER(uint, ProbeGridSize)
			SHADER_PARAMETER(uint, ProbeResolution)
			SHADER_PARAMETER_STRUCT(GPUSceneData, GPUSceneData)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(DDGIUpdateProbesCS)

	void DDGI::Render(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<RenderScene> renderScene)
	{
		if (!m_probeIrradianceAtlas)
		{
			RHI::ImageSpecification imageSpec;
			imageSpec.debugName = "DDGI.ProbeIrradianceAtlas";
			imageSpec.width = s_ddgiProbeResolution.GetValue() * 1;
			imageSpec.height = s_ddgiProbeResolution.GetValue() * 1;
			imageSpec.format = RHI::PixelFormat::B10G11R11_UFLOAT_PACK32;

			m_probeIrradianceAtlas = RHI::Image::Create(imageSpec);

			RGUtils::ClearImage(renderGraph, renderGraph.AddExternalImage(m_probeIrradianceAtlas), { 0.f });
		}

		RenderGraphImageHandle probeIrradianceAtlasHandle = renderGraph.AddExternalImage(m_probeIrradianceAtlas);

		const auto& uniformBuffers = blackboard.Get<UniformBuffersData>();
		const auto& gpuScene = blackboard.Get<GPUSceneData>();

		renderGraph.AddPass("DDGI.UpdateProbes",
		[&](RenderGraph::Builder& builder) 
		{
			builder.WriteResource(probeIrradianceAtlasHandle);
			builder.ReadResource(uniformBuffers.viewDataBuffer);

			BuildGPUSceneData(builder, gpuScene);

			builder.SetIsComputePass();
			builder.SetHasSideEffect();
		},
		[=](RenderContext& context)
		{
			auto pipeline = ShaderMap::GetComputePipeline<DDGIUpdateProbesCS>();

			context.BindPipeline(pipeline);

			context.SetAccelerationStructure(renderScene->GetRayTracingScene()->GetAccelerationStructure());

			DDGIUpdateProbesCS::Parameters parameters;
			parameters.View = uniformBuffers.viewDataBuffer;
			parameters.RWProbeIrradianceAtlas = probeIrradianceAtlasHandle;
			parameters.ProbeSpacing = s_ddgiProbeSpacing.GetValue();
			parameters.ProbeGridSize = static_cast<uint32_t>(s_ddgiProbeGridSize.GetValue());
			parameters.ProbeResolution = static_cast<uint32_t>(s_ddgiProbeResolution.GetValue());
			parameters.GPUSceneData = gpuScene;

			context.SetParameters(parameters);
			context.Dispatch(1, 1, 1);
		});
	}
}
