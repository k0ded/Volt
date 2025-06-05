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

	enum class MeshBatchType : uint8_t
	{
		None = 0,
		VertexIndexBuffer = BIT(0),
		RenderPipeline = BIT(2)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(MeshBatchType);

	class MeshRenderer
	{
	public:
		void BuildRenderCommands(Ref<RenderScene> renderScene);
		void Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters);

	private:
		struct MeshBatch
		{
			RefPtr<RHI::StorageBuffer> vertexBuffer;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
			RefPtr<RHI::DescriptorTable> descriptorTable;

			uint32_t first;
			uint32_t last;

			MeshBatchType batchType;
		};

		struct RenderCommand
		{
			uint32_t indexCount;
			uint32_t firstIndex;
			uint32_t vertexOffset;
		};

		Vector<RenderCommand> m_renderCommands;
		Vector<MeshBatch> m_meshBatches;
	};
}
