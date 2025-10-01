#pragma once

#include "Volt-Renderer/Config.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Core/RHICommon.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>
#include <CoreUtilities/UUID.h>

#include <type_traits>

namespace Volt
{
	struct RenderPrimitiveData;
	class RenderContext;
	class RenderGraph;
	class BatchedShaderParameters;
	class RGBuffer;

	struct MeshDrawCommand
	{
		struct ShaderParameters
		{
			RefPtr<RHI::UniformBuffer> uniformBuffer;
			Map<RHI::ShaderStage, RefPtr<RHI::BufferView>> views;
		};

		RHI::VertexBufferVector vertexBuffers;
		RefPtr<RHI::StorageBuffer> indexBuffer;

		RefPtr<RHI::RenderPipeline> renderPipeline;

		ShaderParameters shaderParameters;
		uint32_t primitiveIndex;
		UUID64 renderPrimitiveID;

		// Draw command
		RHI::DrawIndexedIndirectCommand drawCommand;
	};

	class VTR_API MeshPassProcessor
	{
	public:
		MeshPassProcessor();
		virtual ~MeshPassProcessor() = default;

		void PrepareRenderCommands(RenderGraph& renderGraph);
		void ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters);

		virtual void AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive) = 0;
		virtual void RemoveRenderPrimitive(UUID64 renderPrimitveId) = 0;

	protected:
		void BuildMeshDrawCommand(const RenderPrimitiveData& renderPrimitive, RHI::RenderPipelineCreateInfo pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader);
		void RemoveMeshDrawCommand(UUID64 renderPrimitiveId);

	private:
		MeshDrawCommand::ShaderParameters AllocateShaderParametersForPipeline(RefPtr<RHI::RenderPipeline> renderPipeline);

		Vector<MeshDrawCommand> m_meshDrawCommands;
		Vector<uint32_t> m_perDrawCommandPrimitiveIndices;

		RGBuffer* m_primitiveIndexVertexBuffer = nullptr;
	};

	class MeshPassProcessorRegistry
	{
	public:
		MeshPassProcessorRegistry();
		~MeshPassProcessorRegistry();

		template<typename T>
		requires (std::is_base_of_v<MeshPassProcessor, T>)
		T* AddProcessor()
		{
			constexpr size_t AllocationSize = sizeof(T);

			void* alloc = m_meshPassProcessorAllocator.Allocate(AllocationSize);
			T* processor = new (alloc) T();

			m_meshPassDestructors.emplace_back() = DestructorHelper::Create<T>(alloc);
			m_meshPassProcessors.emplace_back(processor);

			return processor;
		}

		void AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive);
		void RemoveRenderPrimitive(UUID64 renderPrimitiveId);

	private:
		LinearAllocator<> m_meshPassProcessorAllocator;
		Vector<MeshPassProcessor*> m_meshPassProcessors;
		Vector<DestructorHelper> m_meshPassDestructors;
	};
}
