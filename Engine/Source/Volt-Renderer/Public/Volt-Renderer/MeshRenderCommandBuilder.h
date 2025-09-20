#pragma once

#include "Volt-Renderer/Config.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Descriptors/DescriptorTable.h>

namespace Volt
{
	class RenderScene;
	class RenderGraph;
	class RenderMaterial;

	enum class MeshBatchType : uint8_t
	{
		None = 0,
		VertexIndexBuffer = BIT(0),
		RenderPipeline = BIT(2),
		SubMesh = BIT(3)
	};
	VT_SETUP_ENUM_CLASS_OPERATORS(MeshBatchType);

	class VTR_API MeshRenderCommandBuilder
	{
	public:
		struct MeshBatch
		{
			using VertexBufferVector = Vector<RefPtr<RHI::StorageBuffer>, InlineAllocator<32>>;

			VertexBufferVector vertexBuffers;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::Shader> pixelShader;
			Weak<RenderMaterial> renderMaterial;

			int32_t drawCommandOffset;
			MeshBatchType batchType;
		};

		struct RenderCommand
		{
			uint32_t primitiveIndex;
		};

		void Build(RenderScene& renderScene, RenderGraph& renderGraph);

		VT_INLINE bool HasRenderCommands() const { return !m_renderCommands.empty(); }
		VT_INLINE uint32_t NumDrawCommands() const { return m_numDrawCommands; }

		VT_INLINE RGBufferRef GetDrawCommandsToCopyIndicesBuffer() const { return m_drawCommandsToCopyIndices; }
		VT_INLINE RGBufferRef GetPrimitiveIndexToDrawCommandIndexBuffer() const { return m_primitiveIndexToDrawCommandIndex; }
		VT_INLINE RGBufferRef GetValidPrimitiveDrawDataIndicesBuffer() const { return m_validPrimitiveDrawDataIndices; }

		VT_INLINE const Vector<RenderCommand>& GetRenderCommands() const { return m_renderCommands; }
		VT_INLINE const Vector<MeshBatch>& GetMeshBatches() const { return m_meshBatches; }

	private:

		Vector<RenderCommand> m_renderCommands;
		Vector<MeshBatch> m_meshBatches;

		uint32_t m_numDrawCommands = 0;

		RGBufferRef m_drawCommandsToCopyIndices = nullptr;
		RGBufferRef m_primitiveIndexToDrawCommandIndex = nullptr;
		RGBufferRef m_validPrimitiveDrawDataIndices = nullptr;
	};
}
