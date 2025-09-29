#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessor.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

#include <RHIModule/Globals.h>

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

	void MeshPassProcessor::ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();

		for (MeshDrawCommand& drawCommand : m_meshDrawCommands)
		{
			commandBuffer->BindPipeline(drawCommand.renderPipeline);
			commandBuffer->BindVertexBuffers(drawCommand.vertexBuffers, 0);
			commandBuffer->BindIndexBuffer(drawCommand.indexBuffer);
			commandBuffer->DrawIndexed(drawCommand.drawCommand.indexCount, drawCommand.drawCommand.instanceCount, drawCommand.drawCommand.firstIndex, drawCommand.drawCommand.vertexOffset, drawCommand.drawCommand.firstInstance);
		}
	}

	void MeshPassProcessor::BuildMeshDrawCommand(const RenderPrimitiveData& renderPrimitive, const RHI::RenderPipelineCreateInfo& pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader)
	{
		RHI::RenderPipelineCreateInfo pipelineInfoCopy = pipelineInfo;
		pipelineInfoCopy.shaders = { vertexShader, pixelShader };

		RefPtr<RHI::RenderPipeline> renderPipeline = PipelineStateCache::GetRenderPipeline(pipelineInfoCopy);
		//RefPtr<RHI::DescriptorTable> descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderPipeline);

		const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

		MeshDrawCommand& newDrawCommand = m_meshDrawCommands.emplace_back();
		newDrawCommand.renderPipeline = renderPipeline;
		//newDrawCommand.descriptorTable = descriptorTable;
		newDrawCommand.shaderParameters = AllocateShaderParametersForPipeline(renderPipeline);
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexPositionsBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexMaterialBuffer());
		newDrawCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer();
		newDrawCommand.drawCommand.indexCount = subMesh.indexCount;
		newDrawCommand.drawCommand.instanceCount = 1;
		newDrawCommand.drawCommand.firstIndex = subMesh.indexCount;
		newDrawCommand.drawCommand.vertexOffset = subMesh.vertexStartOffset;
		newDrawCommand.drawCommand.firstInstance = 0;

		//for (const auto& [shaderStage, bufferView] : newDrawCommand.shaderParameters.views)
		//{
		//	newDrawCommand.descriptorTable->SetBufferView(bufferView, RHI::GetDescriptorSetIndexFromShaderStage(shaderStage), RHI::Globals::SHADER_GLOBALS_BINDING);
		//}
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
