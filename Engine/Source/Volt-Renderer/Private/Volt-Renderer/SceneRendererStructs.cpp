#include "vrpch.h"
#include "Volt-Renderer/SceneRendererStructs.h"

namespace Volt
{
	void GPUSceneData::Build(RenderGraph::Builder& builder, const GPUSceneData& data)
	{
		builder.ReadResource(data.meshesBuffer);
		builder.ReadResource(data.sdfMeshesBuffer);
		builder.ReadResource(data.materialsBuffer);
		builder.ReadResource(data.primitiveDrawDataBuffer);
		builder.ReadResource(data.prevPrimitiveDrawDataBuffer);
		builder.ReadResource(data.sdfPrimitiveDrawDataBuffer);
		builder.ReadResource(data.validPrimitiveDrawDatasBuffer);
		builder.ReadResource(data.lightsBuffer);
		builder.ReadResource(data.bonesBuffer);
	}

	void GPUSceneData::Setup(RenderContext& context, const GPUSceneData& data)
	{
		context.SetConstant("gpuScene.meshesBuffer"_sh, data.meshesBuffer);
		context.SetConstant("gpuScene.sdfMeshesBuffer"_sh, data.sdfMeshesBuffer);
		context.SetConstant("gpuScene.materialsBuffer"_sh, data.materialsBuffer);
		context.SetConstant("gpuScene.primitiveDrawDataBuffer"_sh, data.primitiveDrawDataBuffer);
		context.SetConstant("gpuScene.prevPrimitiveDrawDataBuffer"_sh, data.prevPrimitiveDrawDataBuffer);
		context.SetConstant("gpuScene.sdfPrimitiveDrawDataBuffer"_sh, data.sdfPrimitiveDrawDataBuffer);
		context.SetConstant("gpuScene.bonesBuffer"_sh, data.bonesBuffer);
		context.SetConstant("gpuScene.validPrimitiveDrawDatasBuffer"_sh, data.validPrimitiveDrawDatasBuffer);
		context.SetConstant("gpuScene.lightsBuffer"_sh, data.lightsBuffer);
	}
}
