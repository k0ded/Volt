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
		m_renderCommands = other.m_renderCommands;
		m_meshBatches = other.m_meshBatches;
		m_indirectDrawCommandsBuffer = other.m_indirectDrawCommandsBuffer;
		m_primitiveDrawDataIndirection = other.m_primitiveDrawDataIndirection;
	}

	void MeshRenderer::BuildRenderCommands(RenderGraph& renderGraph, Ref<RenderScene> renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommands(renderGraph, *renderScene, cullingInfo, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommands(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
	{
		constexpr auto func = [](const RenderPrimitiveData& primitive) { return true; };

		BuildRenderCommandsInternal(renderGraph, renderScene, cullingInfo, func, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommandsWithFilter(RenderGraph& renderGraph, Ref<RenderScene> renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommandsWithFilter(renderGraph, *renderScene, cullingInfo, filterFunc, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommandsWithFilter(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommandsInternal(renderGraph, renderScene, cullingInfo, filterFunc, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters) const
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();

		if (m_primitiveDrawDataIndirection)
		{
			batchedShaderParameters.AddBufferParameter("PrimitiveDrawDataIndirection"_sh, RHI::ShaderResourceType::TexelBuffer, renderContext.GetRHIBuffer(m_primitiveDrawDataIndirection)->GetView(RHI::BufferViewDesc{ .bufferFormat = RHI::PixelFormat::R32_UINT }));
		}

		for (const MeshBatch& meshBatch : m_meshBatches)
		{
			if (EnumValueContainsFlag(meshBatch.batchType, MeshBatchType::RenderPipeline))
			{
				batchedShaderParameters.BindParametersToDescriptorTable(meshBatch.renderPipeline->GetShaderParameterMaps(), meshBatch.descriptorTable);

				commandBuffer->BindPipeline(meshBatch.renderPipeline);
				commandBuffer->BindDescriptorTable(meshBatch.descriptorTable);
			}

			if (EnumValueContainsFlag(meshBatch.batchType, MeshBatchType::VertexIndexBuffer))
			{
				commandBuffer->BindVertexBuffers(meshBatch.vertexBuffers, 0);
				commandBuffer->BindIndexBuffer(meshBatch.indexBuffer);
			}

			if (meshBatch.drawCommandOffset >= 0)
			{
				commandBuffer->DrawIndexedIndirect(renderContext.GetRHIBuffer(m_indirectDrawCommandsBuffer), meshBatch.drawCommandOffset * sizeof(RHI::DrawIndexedIndirectCommand), 1, 0);
			}
		}
	}

	void SetMaterialParametersInDescriptorTable(Weak<RenderMaterial> material, RefPtr<RHI::RenderPipeline> renderPipeline, RefPtr<RHI::DescriptorTable> descriptorTable)
	{
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
			const RHI::ShaderParameterMap::ResourceBindingsMap& bindingsMap = parameterMap.GetResourceBindings();
			for (const auto& [hashedName, binding] : bindingsMap)
			{
				if (binding.resourceType == RHI::ShaderResourceType::Sampler)
				{
					RefPtr<RHI::SamplerState> sampler = SamplerStateCache::GetAnisotropicSampler();

					descriptorTable->SetSamplerState(sampler, binding.set, binding.binding);
				}
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

	void MeshRenderer::BuildRenderCommandsInternal(RenderGraph& renderGraph, RenderScene& renderScene, const CullingInfo& cullingInfo, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
	{
		VT_PROFILE_FUNCTION();

		m_renderCommands.clear();
		m_meshBatches.clear();

		struct RenderCommandExt : public RenderCommand
		{
			size_t vertexIndexBufferHash;
			size_t subMeshHash;
			size_t renderPipelineHash;
			
			uint32_t subMeshIndex;
			MeshBatch::VertexBufferVector vertexBuffers;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
			Weak<RenderMaterial> renderMaterial;
			Weak<Mesh> mesh;
		};

		Vector<RenderCommandExt> renderCommandExts;
		renderCommandExts.reserve(renderScene.GetNumRenderPrimitives());

		Vector<uint32_t> validPrimitiveIndices;
		validPrimitiveIndices.reserve(renderScene.GetNumRenderPrimitives());

		// Counter
		validPrimitiveIndices.push_back(0);

		const bool usePrimitivePixelShader = pixelShader == nullptr;
		const RHI::ShaderInfo& vertexShaderInfo = vertexShader->GetShaderInfo();

		// First get all commands, their info and the required hashes for sorting.
		for (const RenderPrimitiveData& renderPrimitive : renderScene)
		{
			if (!filterFunc(renderPrimitive))
			{
				continue;
			}

			const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

			auto& newCommand = renderCommandExts.emplace_back();
			newCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer()->GetResource();
			newCommand.primitiveIndex = renderScene.GetPrimitiveIndexFromID(renderPrimitive.id);
			newCommand.mesh = renderPrimitive.mesh;
			newCommand.subMeshIndex = renderPrimitive.subMeshIndex;

			for (const auto& [index, layout] : vertexShaderInfo.vertexLayout)
			{
				if (index == 0)
				{
					newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexPositionsBuffer()->GetResource());
				}
				else if (index == 1)
				{
					newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexMaterialBuffer()->GetResource());
				}
				else if (index == 2)
				{
					newCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexAnimationInfoBuffer()->GetResource());
				}
			}

			RHI::RenderPipelineCreateInfo renderPipelineInfo = pipelineInfo;

			RefPtr<RHI::Shader> primitivePixelShader;

			// No pixel shader means that we will use the materials shader.
			if (!pixelShader)
			{
				primitivePixelShader = renderPrimitive.material->GetPixelShader();
			}
			else
			{
				primitivePixelShader = pixelShader;
			}

			// If there still is no pixel shader, we will use the default one
			if (!primitivePixelShader)
			{
				primitivePixelShader = ShaderMap::Get<OpaqueDefaultPixelPS>();
			}

			renderPipelineInfo.shaders = { vertexShader, primitivePixelShader };
			newCommand.renderPipeline = PipelineStateCache::GetRenderPipeline(renderPipelineInfo);
			newCommand.renderMaterial = renderPrimitive.material;

			newCommand.vertexIndexBufferHash = newCommand.indexBuffer.GetHash();

			for (const auto& vertexBuffer : newCommand.vertexBuffers)
			{
				newCommand.vertexIndexBufferHash = Math::HashCombine(newCommand.vertexIndexBufferHash, vertexBuffer.GetHash());
			}

			newCommand.subMeshHash = subMesh.GetHash();
			newCommand.renderPipelineHash = newCommand.renderPipeline->GetHash();

			validPrimitiveIndices[0]++;
			validPrimitiveIndices.push_back(newCommand.primitiveIndex);
		}

		// Now sort the commands to get the correct order
		std::sort(renderCommandExts.begin(), renderCommandExts.end(), [](const RenderCommandExt& lhs, const RenderCommandExt& rhs)
		{
			if (lhs.vertexIndexBufferHash == rhs.vertexIndexBufferHash)
			{
				if (lhs.subMeshHash == rhs.subMeshHash)
				{
					return lhs.renderPipelineHash < rhs.renderPipelineHash;
				}

				return lhs.subMeshHash < rhs.subMeshHash;
			}

			return lhs.vertexIndexBufferHash < rhs.vertexIndexBufferHash;
		});

		// And lastly we build the ranges and put the render commands into the final container.
		m_renderCommands.reserve(renderCommandExts.size());

		size_t lastVertexIndexBufferHash = 0;
		size_t lastRenderPipelineHash = 0;

		struct DefaultInvalid
		{
			int32_t value = -1;
		};

		Vector<uint32_t> meshIds;
		Vector<size_t> renderPipelineHashes;
		Vector<DefaultInvalid> primitiveDrawCommandIndex;

		for (size_t i = 0; i < renderCommandExts.size(); ++i)
		{
			const RenderCommandExt& renderCommandExt = renderCommandExts.at(i);
			const uint32_t meshId = renderScene.GetMeshID(renderCommandExt.mesh, renderCommandExt.subMeshIndex);

			if (i == 0)
			{
				meshIds.emplace_back(meshId);
				renderPipelineHashes.emplace_back(renderCommandExt.renderPipelineHash);
			}
			else
			{
				if (meshId != meshIds.back() || (renderCommandExt.renderPipelineHash != renderPipelineHashes.back() && usePrimitivePixelShader))
				{
					meshIds.emplace_back(meshId);
					renderPipelineHashes.emplace_back(renderCommandExt.renderPipelineHash);
				}
			}

			primitiveDrawCommandIndex.resize(std::max(renderCommandExt.primitiveIndex + 1, static_cast<uint32_t>(primitiveDrawCommandIndex.size())));
			primitiveDrawCommandIndex[renderCommandExt.primitiveIndex].value = static_cast<int32_t>(meshIds.size() - 1);
		}

		if (!meshIds.empty())
		{
			renderGraph.BeginMarker("MeshRenderer::BuildRenderCommands");

			RGBufferRef drawCommandsToCopyIndices = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshIds.size(), "MeshRenderer.DrawCommandsToCopyIndices", RHI::MemoryUsage::CPUToGPU));
			AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(drawCommandsToCopyIndices, RHI::PixelFormat::R32_UINT), meshIds.data(), meshIds.byte_size());

			RGBufferRef primitiveIndexToDrawCommandIndex = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<int32_t>(primitiveDrawCommandIndex.size(), "MeshRenderer.PrimitiveIndexToDrawCommandIndex", RHI::MemoryUsage::CPUToGPU));
			AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(primitiveIndexToDrawCommandIndex, RHI::PixelFormat::R32_SINT), primitiveDrawCommandIndex.data(), primitiveDrawCommandIndex.byte_size());

			RGBufferRef validPrimitiveDrawDataIndices = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(validPrimitiveIndices.size(), "MeshRenderer.ValidPrimitiveIndices", RHI::MemoryUsage::CPUToGPU));
			AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(validPrimitiveDrawDataIndices, RHI::PixelFormat::R32_UINT), validPrimitiveIndices.data(), validPrimitiveIndices.byte_size());

			RGBufferRef drawCommands = renderGraph.CreateBuffer(RGBufferDesc::CreateIndirectDesc<RHI::DrawIndexedIndirectCommand>(meshIds.size(), "MeshRenderer.DrawCommands"));

			// Copy draw commands
			{
				CopyIndirectDrawCommandsCS::Parameters* passParameters = renderGraph.AllocParameters<CopyIndirectDrawCommandsCS::Parameters>();
				passParameters->RWDrawCommands = renderGraph.CreateUAV(drawCommands, RHI::PixelFormat::R32_UINT);
				passParameters->PrebuiltDrawCommands = renderGraph.CreateSRV(renderScene.GetGPUSceneBuffers().perMeshIndirectDrawCommands, RHI::PixelFormat::R32_UINT);
				passParameters->DrawCommandsToCopyIndices = renderGraph.CreateSRV(drawCommandsToCopyIndices, RHI::PixelFormat::R32_UINT);
				passParameters->NumCommandsToCopy = static_cast<uint32_t>(meshIds.size());

				auto shader = ShaderMap::Get<CopyIndirectDrawCommandsCS>();
				ComputeShaderUtils::AddPass<CopyIndirectDrawCommandsCS>(renderGraph,
					"CopyDrawCommands",
					shader,
					passParameters,
					{ Math::DivideRoundUp(meshIds.size(), 64ull), 1, 1 });
			}

			// Cull render primitives
			RGBufferRef perMeshDrawCommandCount = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshIds.size(), "MeshRenderer.PerMeshDrawCommandCount"));
			RGBufferRef primitivesToDrawCounter = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(1, "MeshRenderer.PrimitivesToDrawCounter"));
			RGBufferRef primitivesToDraw = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(renderScene.GetNumRenderPrimitives(), "MeshRenderer.PrimitivesToDraw"));
			{
				AddClearUAVPass(renderGraph, renderGraph.CreateUAV(perMeshDrawCommandCount, RHI::PixelFormat::R32_UINT), 0u);
				AddClearUAVPass(renderGraph, renderGraph.CreateUAV(primitivesToDrawCounter, RHI::PixelFormat::R32_UINT), 0u);

				CullRenderPrimitivesCS::Parameters* passParameters = renderGraph.AllocParameters<CullRenderPrimitivesCS::Parameters>();
				passParameters->RWPerMeshDrawCommandCount = renderGraph.CreateUAV(perMeshDrawCommandCount, RHI::PixelFormat::R32_UINT);
				passParameters->RWPrimitivesToDrawCounter = renderGraph.CreateUAV(primitivesToDrawCounter, RHI::PixelFormat::R32_UINT);
				passParameters->RWPrimitivesToDraw = renderGraph.CreateUAV(primitivesToDraw, RHI::PixelFormat::R32_UINT);
				passParameters->ValidPrimitiveDrawDataIndices = renderGraph.CreateSRV(validPrimitiveDrawDataIndices, RHI::PixelFormat::R32_UINT);
				passParameters->PrimitiveIndexToDrawCommandIndex = renderGraph.CreateSRV(primitiveIndexToDrawCommandIndex, RHI::PixelFormat::R32_SINT);
				passParameters->GPUScene = renderScene.GetGPUSceneParameters(renderGraph);
				passParameters->ViewMatrix = cullingInfo.viewMatrix;
				passParameters->CullingFrustum = cullingInfo.cullingFrustum;
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
			RGBufferRef primitiveStartOffset = renderGraph.CreateBuffer(RGBufferDesc::CreateBufferDesc<uint32_t>(meshIds.size(), "MeshRenderer.PrimitiveStartOffset"));
			{
				PrefixSumTechnique prefixSum{ renderGraph };
				prefixSum.Execute(perMeshDrawCommandCount, primitiveStartOffset, static_cast<uint32_t>(meshIds.size()));
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
				passParameters->PrimitiveIndexToDrawCommandIndex = renderGraph.CreateSRV(primitiveIndexToDrawCommandIndex, RHI::PixelFormat::R32_SINT);

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

		MeshBatch* currentMeshBatch = nullptr;

		for (size_t i = 0; i < renderCommandExts.size(); ++i)
		{
			const RenderCommandExt& renderCommandExt = renderCommandExts.at(i);

			if (i == 0)
			{
				currentMeshBatch = &m_meshBatches.emplace_back();
				currentMeshBatch->batchType = MeshBatchType::VertexIndexBuffer | MeshBatchType::RenderPipeline;
				currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
				currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
				currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
				currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);
				currentMeshBatch->drawCommandOffset = 0;

				SetMaterialParametersInDescriptorTable(renderCommandExt.renderMaterial, renderCommandExt.renderPipeline, currentMeshBatch->descriptorTable);

				lastVertexIndexBufferHash = renderCommandExt.vertexIndexBufferHash;
				lastRenderPipelineHash = renderCommandExt.renderPipelineHash;
			}
			else
			{
				MeshBatchType batchType = MeshBatchType::None;

				if (renderCommandExt.vertexIndexBufferHash != lastVertexIndexBufferHash)
				{
					batchType |= MeshBatchType::VertexIndexBuffer;
					lastVertexIndexBufferHash = renderCommandExt.vertexIndexBufferHash;
				}

				if (renderCommandExt.renderPipelineHash != lastRenderPipelineHash)
				{
					batchType |= MeshBatchType::RenderPipeline;
					lastRenderPipelineHash = renderCommandExt.renderPipelineHash;
				}

				if (batchType != MeshBatchType::None)
				{
					currentMeshBatch = &m_meshBatches.emplace_back();
					currentMeshBatch->batchType = batchType;
					currentMeshBatch->drawCommandOffset = primitiveDrawCommandIndex[renderCommandExt.primitiveIndex].value;

					if (EnumValueContainsFlag(batchType, MeshBatchType::VertexIndexBuffer))
					{
						currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
						currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
					}

					if (EnumValueContainsFlag(batchType, MeshBatchType::RenderPipeline))
					{
						currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
						currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);

						SetMaterialParametersInDescriptorTable(renderCommandExt.renderMaterial, renderCommandExt.renderPipeline, currentMeshBatch->descriptorTable);
					}
				}
			}

			m_renderCommands.emplace_back(renderCommandExt);
		}
	}
}
