#pragma once

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Core.h>

namespace Volt
{
	class RenderScene;
	class RenderContext;
	class BatchedShaderParameters;

	class MeshRenderer
	{
	public:
		void BuildRenderCommands(Ref<RenderScene> renderScene);
		void Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters);

	private:
		struct RenderCommand
		{
			RefPtr<RHI::StorageBuffer> vertexBuffer;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
			RefPtr<RHI::DescriptorTable> descriptorTable;

			uint32_t indexCount;
			uint32_t firstIndex;
			uint32_t vertexOffset;
		};

		PagedVector<RenderCommand> m_renderCommands;
	};
}
