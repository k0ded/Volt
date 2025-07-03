#pragma once

#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Allocators/InlineAllocator.h>

#include <glm/glm.hpp>

namespace Volt
{
	class RenderScene;
	class RenderContext;
	class RenderGraph;
	class BatchedShaderParameters;
	struct RenderPrimitiveData;

	enum class MeshBatchType : uint8_t
	{
		None = 0,
		VertexIndexBuffer = BIT(0),
		RenderPipeline = BIT(2)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(MeshBatchType);

	struct CullingInfo
	{
		enum class Type : uint32_t
		{
			Perspective = 0,
			Orthographic
		};

		Type type = Type::Perspective;
		glm::mat4 viewMatrix;
		glm::vec4 cullingFrustum;
		float nearPlane;
		float farPlane;
	};

	class VTR_API MeshRenderer
	{
	public:
		MeshRenderer() = default;
		MeshRenderer(const MeshRenderer& other) noexcept;

		using PrimitveFilterFunc = std::function<bool(const RenderPrimitiveData&)>;

		// For now we pass a vertex and pixel shader in here, we might want to use vertex shaders specific to a material in the future.
		// A nullptr pixelShader will use the meshes material shader.
		void BuildRenderCommands(RenderGraph& renderGraph, Ref<RenderScene> renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader = nullptr, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});
		void BuildRenderCommands(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader = nullptr, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		void BuildRenderCommandsWithFilter(RenderGraph& renderGraph, Ref<RenderScene> renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});
		void BuildRenderCommandsWithFilter(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		void Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters) const;

	private:
		void BuildRenderCommandsInternal(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo = {});

		struct MeshBatch
		{
			using VertexBufferVector = Vector<RefPtr<RHI::StorageBuffer>, InlineAllocator<32>>;

			VertexBufferVector vertexBuffers;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
			RefPtr<RHI::DescriptorTable> descriptorTable;

			int32_t drawCommandOffset;
			MeshBatchType batchType;
		};

		struct RenderCommand
		{
			uint32_t primitiveIndex;
		};

		Vector<RenderCommand> m_renderCommands;
		Vector<MeshBatch> m_meshBatches;

		RGBufferRef m_indirectDrawCommandsBuffer = nullptr;
		RGBufferRef m_primitiveDrawDataIndirection = nullptr;
	};
}
