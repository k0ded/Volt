#include "vrpch.h"

#include "Volt-Renderer/Mesh/MeshRenderer.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Texture/Texture2D.h"
#include "Volt-Renderer/RenderingTechniques/PrefixSumTechnique.h"

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/DescriptorTableCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/Shader/DefaultShaders.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt
{

	MeshRenderer::MeshRenderer(const MeshRenderer& other) noexcept
	{
		m_indirectDrawCommandsBuffer = other.m_indirectDrawCommandsBuffer;
		m_primitiveDrawDataIndirection = other.m_primitiveDrawDataIndirection;
		m_vertexShader = other.m_vertexShader;
		m_pixelShader = other.m_pixelShader;
		m_renderPipelineCreateInfo = other.m_renderPipelineCreateInfo;
	}

	void MeshRenderer::BuildRenderCommands(RenderGraph& renderGraph, Ref<RenderScene> renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommands(renderGraph, *renderScene, cullingInfo, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommands(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
	{
		BuildRenderCommandsInternal(renderGraph, renderScene, cullingInfo, vertexShader, pixelShader, pipelineInfo);
	}

	void SetMaterialParametersInDescriptorTable(Weak<RenderMaterial> material, RefPtr<RHI::RenderPipeline> renderPipeline, RefPtr<RHI::DescriptorTable> descriptorTable)
	{
		VT_PROFILE_FUNCTION();

		const auto& materialTextures = material->GetTextures();

		const Vector<RHI::ShaderParameterMap>& shaderParameterMaps = renderPipeline->GetShaderParameterMaps();

		for (const auto& [index, materialTexture] : materialTextures)
		{
			for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(StringHash::Construct(materialTexture.bindingName));
				if (resourceBinding)
				{
					auto image = materialTexture.texture.GetResource();

					if (!image)
					{
						image = Renderer::GetDefaultResources().whiteTexture->GetImage();
					}

					descriptorTable->SetImageView(image->GetView(), resourceBinding->set, resourceBinding->binding);
				}
			}
		}

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			const RHI::ShaderParameterMap::ResourceBindings& bindingsMap = parameterMap.GetResourceBindings();
			for (const auto& [binding, hashedName] : bindingsMap)
			{
				if (binding.resourceType == RHI::ShaderResourceType::Sampler)
				{
					RefPtr<RHI::SamplerState> sampler = SamplerStateCache::GetAnisotropicSampler();

					descriptorTable->SetSamplerState(sampler, binding.set, binding.binding);
				}
			}
		}
	}

	void MeshRenderer::Render(RenderContext& renderContext, RenderScene& renderScene, BatchedShaderParameters& batchedShaderParameters) const
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();

		if (m_primitiveDrawDataIndirection)
		{
			batchedShaderParameters.AddBufferParameter("PrimitiveDrawDataIndirection"_sh, RHI::ShaderResourceType::TexelBuffer, m_primitiveDrawDataIndirection->GetRHIResource()->GetOrCreateView(RHI::BufferViewDesc{ .bufferFormat = RHI::PixelFormat::R32_UINT }));
		}

		const bool shouldOverrideMaterials = m_vertexShader && m_pixelShader;

		const MeshRenderCommandBuilder& meshRenderCommandBuilder = renderScene.GetMeshRenderCommandBuilder();

		RefPtr<RHI::RenderPipeline> activeRenderPipeline;
		RefPtr<RHI::DescriptorTable> activeDescriptorTable;

		RHI::RenderPipelineCreateInfo tempRenderPipelineInfo = m_renderPipelineCreateInfo;
		tempRenderPipelineInfo.shaders.resize(2);
		tempRenderPipelineInfo.shaders[0] = m_vertexShader;

		if (shouldOverrideMaterials)
		{
			tempRenderPipelineInfo.shaders[1] = m_pixelShader;
			activeRenderPipeline = PipelineStateCache::GetRenderPipeline(tempRenderPipelineInfo);
		}

		for (const MeshRenderCommandBuilder::MeshBatch& meshBatch : meshRenderCommandBuilder.GetMeshBatches())
		{
			VT_PROFILE_SCOPE("Draw Mesh batch");

			if (EnumValueContainsFlag(meshBatch.batchType, MeshBatchType::RenderPipeline))
			{
				if (!shouldOverrideMaterials)
				{
					tempRenderPipelineInfo.shaders[1] = meshBatch.pixelShader;
					activeRenderPipeline = PipelineStateCache::GetRenderPipeline(tempRenderPipelineInfo);
				}
				activeDescriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(activeRenderPipeline);
				SetMaterialParametersInDescriptorTable(meshBatch.renderMaterial, activeRenderPipeline, activeDescriptorTable);

				InlineVector<RenderContext::PerStageShaderParameters, 8> perStageParameters = renderContext.AllocatePerStageShaderParameterBuffers(activeRenderPipeline);

				batchedShaderParameters.PopulateShaderParameterUniformBuffers(activeRenderPipeline->GetShaderParameterMaps(), perStageParameters);
				batchedShaderParameters.BindShaderBindingsToDescriptorTable(activeRenderPipeline->GetShaderParameterMaps(), activeDescriptorTable, perStageParameters);

				commandBuffer->BindPipeline(activeRenderPipeline);
				commandBuffer->BindDescriptorTable(activeDescriptorTable);
			}

			if (EnumValueContainsFlag(meshBatch.batchType, MeshBatchType::VertexIndexBuffer))
			{
				commandBuffer->BindVertexBuffers(meshBatch.vertexBuffers, 0);
				commandBuffer->BindIndexBuffer(meshBatch.indexBuffer);
			}

			if (meshBatch.drawCommandOffset >= 0)
			{
				commandBuffer->DrawIndexedIndirect(m_indirectDrawCommandsBuffer->GetRHIResource()->GetRHIBuffer(), meshBatch.drawCommandOffset * sizeof(RHI::DrawIndexedIndirectCommand), 1, 0);
			}
		}
	}

	struct CopyIndirectDrawCommandsCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CopyIndirectDrawCommandsCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWDrawCommands)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, PrebuiltDrawCommands)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, DrawCommandsToCopyIndices)
			SHADER_PARAMETER(uint, NumCommandsToCopy)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CopyIndirectDrawCommandsCS, "Engine/Shaders/Source/RenderPipelineLegacy/CullAndWriteRenderCommands.hlsl", "CopyIndirectDrawCommandsCS", Compute);

	struct CullRenderPrimitivesCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CullRenderPrimitivesCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWPerMeshDrawCommandCount)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWPrimitivesToDrawCounter)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWPrimitivesToDraw)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, ValidPrimitiveDrawDataIndices)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, PrimitiveIndexToDrawCommandIndex)
			SHADER_PARAMETER_STRUCT_INCLUDE(GPUSceneParameters, GPUScene)
			SHADER_PARAMETER(float4x4, ViewMatrix)
			SHADER_PARAMETER(float4, CullingFrustum)
			SHADER_PARAMETER(float, NearPlane)
			SHADER_PARAMETER(float, FarPlane)
			SHADER_PARAMETER(uint, CullingType)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CullRenderPrimitivesCS, "Engine/Shaders/Source/RenderPipelineLegacy/CullAndWriteRenderCommands.hlsl", "CullRenderPrimitivesCS", Compute);

	struct CompactAndWriteRenderCommandsCS : public GlobalShader
	{
		DECLARE_GLOBAL_SHADER(CompactAndWriteRenderCommandsCS)
		BEGIN_SHADER_PARAMETER_STRUCT(Parameters)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWPrimitiveDrawDataIndirection)
			SHADER_PARAMETER_BUFFER_UAV(RWBuffer<uint>, RWDrawCommands)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, PrimitiveStartOffset)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, PrimitivesToDraw)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<uint>, PrimitivesToDrawCounter)
			SHADER_PARAMETER_BUFFER_SRV(Buffer<int>, PrimitiveIndexToDrawCommandIndex)
		END_SHADER_PARAMETER_STRUCT()
	};
	REGISTER_SHADER(CompactAndWriteRenderCommandsCS, "Engine/Shaders/Source/RenderPipelineLegacy/CullAndWriteRenderCommands.hlsl", "CompactAndWriteRenderCommandsCS", Compute);

	void MeshRenderer::BuildRenderCommandsInternal(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
	{
		VT_PROFILE_FUNCTION();

		m_vertexShader = vertexShader;
		m_pixelShader = pixelShader;
		m_renderPipelineCreateInfo = pipelineInfo;

		const MeshRenderCommandBuilder& meshRenderCommandBuilder = renderScene.GetMeshRenderCommandBuilder();

		if (meshRenderCommandBuilder.HasRenderCommands())
		{
			renderGraph.BeginMarker("MeshRenderer::BuildRenderCommands");

			RGBufferRef drawCommands = renderGraph.CreateBuffer(RGBufferDesc::CreateIndirectDesc<RHI::DrawIndexedIndirectCommand>(meshRenderCommandBuilder.NumDrawCommands(), "MeshRenderer.DrawCommands"));

			// Copy draw commands
			{
				CopyIndirectDrawCommandsCS::Parameters* passParameters = renderGraph.AllocParameters<CopyIndirectDrawCommandsCS::Parameters>();
				passParameters->RWDrawCommands = renderGraph.CreateUAV(drawCommands, RHI::PixelFormat::R32_UINT);
				passParameters->PrebuiltDrawCommands = renderGraph.CreateSRV(renderScene.GetGPUSceneBuffers().perMeshIndirectDrawCommands, RHI::PixelFormat::R32_UINT);
				passParameters->DrawCommandsToCopyIndices = renderGraph.CreateSRV(meshRenderCommandBuilder.GetDrawCommandsToCopyIndicesBuffer(), RHI::PixelFormat::R32_UINT);
				passParameters->NumCommandsToCopy = meshRenderCommandBuilder.NumDrawCommands();

				auto shader = ShaderMap::Get<CopyIndirectDrawCommandsCS>();
				ComputeShaderUtils::AddPass<CopyIndirectDrawCommandsCS>(renderGraph,
					"CopyDrawCommands",
					shader,
					passParameters,
					{ Math::DivideRoundUp(meshRenderCommandBuilder.NumDrawCommands(), 64u), 1, 1 });
			}

			// Cull render primitives
			RGBufferRef perMeshDrawCommandCount = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshRenderCommandBuilder.NumDrawCommands(), "MeshRenderer.PerMeshDrawCommandCount"));
			RGBufferRef primitivesToDrawCounter = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1, "MeshRenderer.PrimitivesToDrawCounter"));
			RGBufferRef primitivesToDraw = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(renderScene.GetNumRenderPrimitives(), "MeshRenderer.PrimitivesToDraw"));
			{
				AddClearUAVPass(renderGraph, renderGraph.CreateUAV(perMeshDrawCommandCount, RHI::PixelFormat::R32_UINT), 0u);
				AddClearUAVPass(renderGraph, renderGraph.CreateUAV(primitivesToDrawCounter, RHI::PixelFormat::R32_UINT), 0u);

				CullRenderPrimitivesCS::Parameters* passParameters = renderGraph.AllocParameters<CullRenderPrimitivesCS::Parameters>();
				passParameters->RWPerMeshDrawCommandCount = renderGraph.CreateUAV(perMeshDrawCommandCount, RHI::PixelFormat::R32_UINT);
				passParameters->RWPrimitivesToDrawCounter = renderGraph.CreateUAV(primitivesToDrawCounter, RHI::PixelFormat::R32_UINT);
				passParameters->RWPrimitivesToDraw = renderGraph.CreateUAV(primitivesToDraw, RHI::PixelFormat::R32_UINT);
				passParameters->ValidPrimitiveDrawDataIndices = renderGraph.CreateSRV(meshRenderCommandBuilder.GetValidPrimitiveDrawDataIndicesBuffer(), RHI::PixelFormat::R32_UINT);
				passParameters->PrimitiveIndexToDrawCommandIndex = renderGraph.CreateSRV(meshRenderCommandBuilder.GetPrimitiveIndexToDrawCommandIndexBuffer(), RHI::PixelFormat::R32_SINT);
				passParameters->GPUScene = renderScene.GetGPUSceneParameters(renderGraph);
				passParameters->ViewMatrix = cullingInfo.viewMatrix;
				passParameters->CullingFrustum = cullingInfo.cullingFrustum;
				passParameters->CullingType = static_cast<uint32_t>(cullingInfo.type);
				passParameters->NearPlane = cullingInfo.nearPlane;
				passParameters->FarPlane = cullingInfo.farPlane;

				auto shader = ShaderMap::Get<CullRenderPrimitivesCS>();
				ComputeShaderUtils::AddPass<CullRenderPrimitivesCS>(renderGraph,
					"CullRenderPrimitives",
					shader,
					passParameters,
					{ Math::DivideRoundUp(renderScene.GetNumRenderPrimitives(), 256u), 1, 1 });
			}

			// Prefix Sum
			RGBufferRef primitiveStartOffset = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshRenderCommandBuilder.NumDrawCommands(), "MeshRenderer.PrimitiveStartOffset"));
			{
				PrefixSumTechnique prefixSum{ renderGraph };
				prefixSum.Execute(perMeshDrawCommandCount, primitiveStartOffset, meshRenderCommandBuilder.NumDrawCommands());
			}

			// Compact and write render commands
			RGBufferRef primitiveDrawDataIndirection = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(renderScene.GetNumRenderPrimitives(), "MeshRenderer.PrimitiveDrawDataIndirection"));
			{
				CompactAndWriteRenderCommandsCS::Parameters* passParameters = renderGraph.AllocParameters<CompactAndWriteRenderCommandsCS::Parameters>();
				passParameters->RWPrimitiveDrawDataIndirection = renderGraph.CreateUAV(primitiveDrawDataIndirection, RHI::PixelFormat::R32_UINT);
				passParameters->RWDrawCommands = renderGraph.CreateUAV(drawCommands, RHI::PixelFormat::R32_UINT);
				passParameters->PrimitiveStartOffset = renderGraph.CreateSRV(primitiveStartOffset, RHI::PixelFormat::R32_UINT);
				passParameters->PrimitivesToDraw = renderGraph.CreateSRV(primitivesToDraw, RHI::PixelFormat::R32_UINT);
				passParameters->PrimitivesToDrawCounter = renderGraph.CreateSRV(primitivesToDrawCounter, RHI::PixelFormat::R32_UINT);
				passParameters->PrimitiveIndexToDrawCommandIndex = renderGraph.CreateSRV(meshRenderCommandBuilder.GetPrimitiveIndexToDrawCommandIndexBuffer(), RHI::PixelFormat::R32_SINT);

				auto shader = ShaderMap::Get<CompactAndWriteRenderCommandsCS>();
				ComputeShaderUtils::AddPass<CompactAndWriteRenderCommandsCS>(renderGraph,
					"CompactAndWriteRenderCommands",
					shader,
					passParameters,
					RenderGraphPassFlags::NeverCull,
					{ Math::DivideRoundUp(renderScene.GetNumRenderPrimitives(), 256u), 1, 1 });
			}

			// Indirect draw commands
			{
				RHI::ResourceState dstResourceState;
				dstResourceState.stage = RHI::BarrierStage::DrawIndirect;
				dstResourceState.access = RHI::BarrierAccess::IndirectArgument;
				renderGraph.AddResourceBarrier(drawCommands, dstResourceState);
			}

			// Indirection buffer
			{
				RHI::ResourceState dstResourceState;
				dstResourceState.stage = RHI::BarrierStage::VertexShader;
				dstResourceState.access = RHI::BarrierAccess::ShaderRead;
				renderGraph.AddResourceBarrier(primitiveDrawDataIndirection, dstResourceState);
			}

			m_indirectDrawCommandsBuffer = drawCommands;
			m_primitiveDrawDataIndirection = primitiveDrawDataIndirection;

			renderGraph.EndMarker();
		}
	}
}
