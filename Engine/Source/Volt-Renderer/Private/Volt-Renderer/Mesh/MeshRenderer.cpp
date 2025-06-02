#include "vrpch.h"

#include "Volt-Renderer/Mesh/MeshRenderer.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/DescriptorTableCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

namespace Volt
{
	struct MeshTestVS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(MeshTestVS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_UNIFORM_BUFFER(ConstantBuffer<ViewData>, View)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(MeshTestVS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainVS", Vertex);

	struct MeshTestPS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(MeshTestPS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(MeshTestPS, "Engine/Shaders/Source/RenderPipelineLegacy/DepthPrePass.hlsl", "MainPS", Pixel);

	void MeshRenderer::BuildRenderCommands(Ref<RenderScene> renderScene)
	{
		m_renderCommands.clear();

		for (const RenderPrimitiveData& renderPrimitive : *renderScene)
		{
			const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

			auto& newCommand = m_renderCommands.emplace_back();
			newCommand.vertexBuffer = renderPrimitive.mesh->GetVertexPositionsBuffer()->GetResource();
			newCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer()->GetResource();
			newCommand.indexCount = subMesh.indexCount;
			newCommand.firstIndex = subMesh.indexStartOffset;
			newCommand.vertexOffset = subMesh.vertexStartOffset;
		
			RHI::RenderPipelineCreateInfo renderPipelineInfo{};
			renderPipelineInfo.shaders = { ShaderMap::Get<MeshTestVS>(), ShaderMap::Get<MeshTestPS>() };

			newCommand.renderPipeline = PipelineStateCache::GetRenderPipeline(renderPipelineInfo);
			newCommand.descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(newCommand.renderPipeline);
		}
	}

	void MeshRenderer::Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();
	
		for (const RenderCommand& cmd : m_renderCommands)
		{
			batchedShaderParameters.BindParametersToDescriptorTable(cmd.descriptorTable);

			commandBuffer->BindPipeline(cmd.renderPipeline);
			commandBuffer->BindDescriptorTable(cmd.descriptorTable);
			commandBuffer->BindVertexBuffers({ cmd.vertexBuffer }, 0);
			commandBuffer->BindIndexBuffer(cmd.indexBuffer);
			commandBuffer->DrawIndexed(cmd.indexCount, 1, cmd.firstIndex, cmd.vertexOffset, 0);
		}
	}
}
