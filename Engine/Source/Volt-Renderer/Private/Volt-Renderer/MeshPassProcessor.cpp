#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessor.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>

#include <RHIModule/Globals.h>
#include <RHIModule/RHICapabilities.h>

namespace Volt
{
	MeshPassProcessorRegistry::MeshPassProcessorRegistry()
	{
		m_meshPassProcessorAllocator.Reserve(512 * 1024);
	}

	MeshPassProcessorRegistry::~MeshPassProcessorRegistry()
	{
		for (DestructorHelper& destructorHelper : m_meshPassDestructors)
		{
			destructorHelper.Destroy();
		}
	}

	void MeshPassProcessorRegistry::AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive)
	{
		for (MeshPassProcessor* meshPassProcessor : m_meshPassProcessors)
		{
			meshPassProcessor->AddRenderPrimitive(renderPrimitive);
		}
	}

	void MeshPassProcessorRegistry::RemoveRenderPrimitive(UUID64 renderPrimitiveId)
	{
		for (MeshPassProcessor* meshPassProcessor : m_meshPassProcessors)
		{
			meshPassProcessor->RemoveRenderPrimitive(renderPrimitiveId);
		}
	}

	MeshPassProcessor::MeshPassProcessor()
	{
	}

	void MeshPassProcessor::PrepareRenderCommands(RenderGraph& renderGraph)
	{
		RGBufferDesc bufferDesc{};
		bufferDesc.count = std::max(static_cast<uint32_t>(m_perDrawCommandPrimitiveIndices.size()), 100u);
		bufferDesc.elementSize = sizeof(uint32_t);
		bufferDesc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::StorageBuffer;
		bufferDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		bufferDesc.debugName = "PrimitiveIndexVertexBuffer";

		m_primitiveIndexVertexBuffer = renderGraph.CreateBuffer(bufferDesc);

		// Make sure the buffer is referenced.
		m_primitiveIndexVertexBuffer->AddRef();

		AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_primitiveIndexVertexBuffer), m_perDrawCommandPrimitiveIndices.data(), m_perDrawCommandPrimitiveIndices.byte_size());
	}

	void MeshPassProcessor::ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		if (m_meshDrawCommands.empty())
		{
			return;
		}

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();
		RefPtr<RHI::StorageBuffer> primitiveIndexVertexBuffer = m_primitiveIndexVertexBuffer->GetRHIResource()->GetRHIBuffer();

		RHI::VertexBufferVector primitiveIndexVertexBufferVector;
		primitiveIndexVertexBufferVector.resize(1);
		primitiveIndexVertexBufferVector[0].buffer = primitiveIndexVertexBuffer;

		for (uint64_t offset = 0; MeshDrawCommand& drawCommand : m_meshDrawCommands)
		{
			RHI::ShaderBindingMap shaderBindings;
			batchedShaderParameters.BindShaderBindings(drawCommand.renderPipeline->GetShaderParameterMaps(), shaderBindings);

			VT_ENSURE_MSG(drawCommand.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.layout.IsValid(), "Mesh pass processors must have a per instance layout!");

			const uint32_t perInstanceBindingIndex = drawCommand.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.bindingIndex;

			primitiveIndexVertexBufferVector[0].offset = offset * sizeof(uint32_t);

			commandBuffer->BindPipeline(drawCommand.renderPipeline);
			commandBuffer->BindShaderBindings(shaderBindings);
			commandBuffer->BindVertexBuffers(drawCommand.vertexBuffers, 0);
			commandBuffer->BindVertexBuffers(primitiveIndexVertexBufferVector, perInstanceBindingIndex);
			commandBuffer->BindIndexBuffer(drawCommand.indexBuffer);
			commandBuffer->DrawIndexed(drawCommand.drawCommand.indexCount, drawCommand.drawCommand.instanceCount, drawCommand.drawCommand.firstIndex, drawCommand.drawCommand.vertexOffset, drawCommand.drawCommand.firstInstance);
		
			offset++;
		}
	}

	void MeshPassProcessor::BuildMeshDrawCommand(const RenderPrimitiveData& renderPrimitive, RHI::RenderPipelineCreateInfo pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader)
	{
		pipelineInfo.shaders = { vertexShader, pixelShader };

		RefPtr<RHI::RenderPipeline> renderPipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

		const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

		MeshDrawCommand& newDrawCommand = m_meshDrawCommands.emplace_back();
		newDrawCommand.renderPipeline = renderPipeline;
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexPositionsBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexMaterialBuffer());
		newDrawCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer();
		newDrawCommand.primitiveIndex = renderPrimitive.primitiveIndex;
		newDrawCommand.renderPrimitiveID = renderPrimitive.id;
		newDrawCommand.drawCommand.indexCount = subMesh.indexCount;
		newDrawCommand.drawCommand.instanceCount = 1;
		newDrawCommand.drawCommand.firstIndex = subMesh.indexStartOffset;
		newDrawCommand.drawCommand.vertexOffset = subMesh.vertexStartOffset;
		newDrawCommand.drawCommand.firstInstance = 0;

		m_perDrawCommandPrimitiveIndices.emplace_back(renderPrimitive.primitiveIndex);
	}

	void MeshPassProcessor::RemoveMeshDrawCommand(UUID64 renderPrimitiveId)
	{
		uint32_t primitiveIndex = uint32_t(-1);

		for (size_t i = 0; i < m_meshDrawCommands.size(); ++i)
		{
			if (m_meshDrawCommands.at(i).renderPrimitiveID == renderPrimitiveId)
			{
				primitiveIndex = m_meshDrawCommands.at(i).primitiveIndex;
				m_meshDrawCommands.erase(m_meshDrawCommands.begin() + i);
				break;
			}
		}

		m_perDrawCommandPrimitiveIndices.erase_with_predicate([primitiveIndex](uint32_t index)
		{
			return primitiveIndex == index;
		});
	}

	MeshDrawCommand::ShaderParameters MeshPassProcessor::AllocateShaderParametersForPipeline(RefPtr<RHI::RenderPipeline> renderPipeline)
	{
		VT_PROFILE_FUNCTION();

		ArrayView<RHI::ShaderParameterMap> shaderParameterMaps = renderPipeline->GetShaderParameterMaps();

		// Loop through all shader parameter maps to find the total size and
		// each shader stages offset
		uint32_t totalShaderParameterSize = 0;
		Map<RHI::ShaderStage, uint32_t> perShaderStageOffset;

		for (const RHI::ShaderParameterMap& shaderParameterMap : shaderParameterMaps)
		{
			if (shaderParameterMap.IsValid())
			{
				perShaderStageOffset[shaderParameterMap.GetShaderStage()] = totalShaderParameterSize;
				totalShaderParameterSize += shaderParameterMap.GetShaderParametersSize();
			}
		}

		if (totalShaderParameterSize == 0)
		{
			return {};
		}

		RHI::UniformBufferDesc uboDesc{};
		uboDesc.size = totalShaderParameterSize;
		uboDesc.debugName = "ShaderParameters";

		RefPtr<RHI::UniformBuffer> uniformBuffer = RHI::UniformBuffer::Create(uboDesc);

		MeshDrawCommand::ShaderParameters result;
		result.uniformBuffer = uniformBuffer;

		for (const RHI::ShaderParameterMap& shaderParameterMap : shaderParameterMaps)
		{
			if (shaderParameterMap.IsValid())
			{
				RHI::BufferViewDesc viewDesc{};
				viewDesc.size = shaderParameterMap.GetShaderParametersSize();
				viewDesc.offset = perShaderStageOffset[shaderParameterMap.GetShaderStage()];

				result.views[shaderParameterMap.GetShaderStage()] = uniformBuffer->GetView(viewDesc);
			}
		}

		return result;
	}
}
