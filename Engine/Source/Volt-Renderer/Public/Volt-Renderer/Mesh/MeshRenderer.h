#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Allocators/InlineAllocator.h>

namespace Volt
{
	class RenderScene;
	class RenderContext;
	class BatchedShaderParameters;
	struct RenderPrimitiveData;

	enum class MeshBatchType : uint8_t
	{
		None = 0,
		VertexIndexBuffer = BIT(0),
		RenderPipeline = BIT(2)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(MeshBatchType);

	class VTR_API MeshRenderer
	{
	public:
		MeshRenderer() = default;
		MeshRenderer(const MeshRenderer& other) noexcept;

		using PrimitveFilterFunc = std::function<bool(const RenderPrimitiveData&)>;

		// For now we pass a vertex and pixel shader in here, we might want to use vertex shaders specific to a material in the future.
		void BuildRenderCommands(Ref<RenderScene> renderScene, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});
		void BuildRenderCommands(RenderScene& renderScene, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		void BuildRenderCommandsWithFilter(Ref<RenderScene> renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});
		void BuildRenderCommandsWithFilter(RenderScene& renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		void Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters) const;

	private:
		void BuildRenderCommandsInternal(RenderScene& renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		struct MeshBatch
		{
			using VertexBufferVector = Vector<RefPtr<RHI::StorageBuffer>, InlineAllocator<32>>;

			VertexBufferVector vertexBuffers;
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
			uint32_t primitiveIndex;
		};

		Vector<RenderCommand> m_renderCommands;
		Vector<MeshBatch> m_meshBatches;
	};
}
