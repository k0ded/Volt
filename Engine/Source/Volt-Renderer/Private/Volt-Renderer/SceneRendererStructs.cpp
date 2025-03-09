#include "vrpch.h"
#include "Volt-Renderer/SceneRendererStructs.h"

namespace Volt
{
	void BuildGPUSceneData(RenderGraph::Builder& builder, const GPUSceneData& data)
	{
		builder.ReadResource(data.meshesBuffer);
		builder.ReadResource(data.sdfMeshesBuffer);
		builder.ReadResource(data.materialsBuffer);
		builder.ReadResource(data.primitiveDrawDataBuffer);
		builder.ReadResource(data.prevPrimitiveDrawDataBuffer);
		builder.ReadResource(data.validPrimitiveDrawDatasBuffer);
		builder.ReadResource(data.lightsBuffer);
		builder.ReadResource(data.bonesBuffer);
	}
}
