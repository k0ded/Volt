#include "vrpch.h"

#include "Volt-Renderer/Debug/DebugMeshRenderer.h"
#include "Volt-Renderer/Mesh/Mesh.h"

#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

#include <JobSystem/TaskGraph.h>

#include <RHIModule/Globals.h>

namespace Volt
{
	struct DebugMeshData
	{
		glm::vec3 position;
		uint32_t userData;
		glm::vec3 scale;
		float padding0;
		glm::quat rotation;
	};

	void DebugMeshRenderer::ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
	{
		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();

		RHI::VertexBufferVector primitiveIndexVertexBufferVector;
		primitiveIndexVertexBufferVector.resize(1);
		primitiveIndexVertexBufferVector[0].buffer = m_primitiveIndexDataBuffer->GetRHIResource()->GetRHIBuffer();

		RawPtr<RHI::RenderPipeline> prevRenderPipeline;
		uint32_t primitiveOffset = 0;

		for (const MeshDrawCommandBucket& currentBucket : m_meshDrawCommandBuckets)
		{
			for (const MeshDrawCommandBucket::InstancingRange& instancingRange : currentBucket.instancingRanges)
			{
				const MeshDrawCommandBucket::MeshDrawCommandInfo& drawCommandInfo = currentBucket.drawCommands.at(instancingRange.offset);
				const MeshDrawCommand& firstDrawCommand = drawCommandInfo.drawCommand;

				VT_ENSURE_MSG(firstDrawCommand.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.layout.IsValid(), "Mesh pass processors must have a per instance layout!");
				const uint32_t perInstanceBindingIndex = firstDrawCommand.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.bindingIndex;

				primitiveIndexVertexBufferVector[0].offset = (primitiveOffset + instancingRange.offset) * sizeof(uint32_t);

				// We don't need to rebind the same pipeline.
				const bool shouldBindPipeline = (prevRenderPipeline == nullptr || prevRenderPipeline != firstDrawCommand.renderPipeline);
				if (shouldBindPipeline)
				{
					prevRenderPipeline = firstDrawCommand.renderPipeline;
					commandBuffer->BindPipeline(firstDrawCommand.renderPipeline);

					ArrayView<RHI::ShaderParameterMap> shaderParametersMaps = firstDrawCommand.renderPipeline->GetShaderParameterMaps();
					InlineVector<RenderContext::PerStageShaderParameters, 8> perShaderStageParameters = renderContext.SetupPipelineData(firstDrawCommand.renderPipeline);

					RHI::ShaderBindingMap shaderBindings = RHI::ShaderBindingMap::InitializeFromPipeline(firstDrawCommand.renderPipeline);
					batchedShaderParameters.BindShaderBindings(shaderParametersMaps, shaderBindings);
					batchedShaderParameters.PopulateShaderParameterUniformBuffers(shaderParametersMaps, perShaderStageParameters);

					for (auto& shaderParameters : perShaderStageParameters)
					{
						shaderBindings.SetUniformBufferWithSizeAndOffset(shaderParameters.shaderStage, RHI::Globals::SHADER_GLOBALS_BINDING, shaderParameters.uniformBufferSRV->GetRHIView(), shaderParameters.size, shaderParameters.offset);
					}

					drawCommandInfo.material->BindToShaderBindingMap(shaderBindings, firstDrawCommand.renderPipeline);

					commandBuffer->BindShaderBindings(shaderBindings);
				}

				commandBuffer->BindVertexBuffers(firstDrawCommand.vertexBuffers, 0);
				commandBuffer->BindVertexBuffers(primitiveIndexVertexBufferVector, perInstanceBindingIndex);
				commandBuffer->BindIndexBuffer(firstDrawCommand.indexBuffer);

				MaterialShader::InlineParameterBlock inlineParameterBlock;
				inlineParameterBlock.materialBlendMode = std::to_underlying(drawCommandInfo.material->GetMaterialBlendMode());
				inlineParameterBlock.isDoubleSided = drawCommandInfo.material->GetIsDoubleSided();

				commandBuffer->PushInlineParameters(&inlineParameterBlock, sizeof(MaterialShader::InlineParameterBlock), 0, RHI::ShaderStage::Pixel);
				commandBuffer->DrawIndexed(
					firstDrawCommand.drawCommand.indexCount,
					instancingRange.count,
					firstDrawCommand.drawCommand.firstIndex,
					firstDrawCommand.drawCommand.vertexOffset,
					firstDrawCommand.drawCommand.firstInstance);
			}

			primitiveOffset += static_cast<uint32_t>(currentBucket.drawCommands.size());
		}
	}

	void DebugMeshRenderer::PrepareMeshesForRendering(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		// Create a task graph to sort and find the instancing ranges for each bucket.
		TaskGraph taskGraph{ ExecutionPriority::Render, m_meshDrawCommandBuckets.size() };

		for (MeshDrawCommandBucket& meshDrawCommandBucket : m_meshDrawCommandBuckets)
		{
			// If a bucket only has 1 command, there is no point in launching a task for it.
			if (meshDrawCommandBucket.drawCommands.size() == 1)
			{
				MeshDrawCommandBucket::InstancingRange& instancingRange = meshDrawCommandBucket.instancingRanges.emplace_back();
				instancingRange.count = 1;
				instancingRange.offset = 0;
			}
			else
			{
				taskGraph.AddTask("Sort and find instancing ranges", [&meshDrawCommandBucket]() 
				{
					Vector<MeshDrawCommandBucket::MeshDrawCommandInfo>& meshDrawCommandInfos = meshDrawCommandBucket.drawCommands;
					Vector<MeshDrawCommandBucket::InstancingRange>& instancingRanges = meshDrawCommandBucket.instancingRanges;

					// Sort
					std::sort(meshDrawCommandInfos.begin(), meshDrawCommandInfos.end(), [](const MeshDrawCommandBucket::MeshDrawCommandInfo& lhs, const MeshDrawCommandBucket::MeshDrawCommandInfo& rhs)
					{
						return lhs.drawCommand.sortKey.sortKey < rhs.drawCommand.sortKey.sortKey;
					});

					// Find instancing ranges
					instancingRanges.clear();

					for (size_t index = 0; index < meshDrawCommandInfos.size(); ++index)
					{
						if (index == 0)
						{
							instancingRanges.emplace_back(0, 1);
						}
						else
						{
							const MeshDrawCommandBucket::MeshDrawCommandInfo& prevDrawCommand = meshDrawCommandInfos.at(index - 1);
							const MeshDrawCommandBucket::MeshDrawCommandInfo& currDrawCommand = meshDrawCommandInfos.at(index);
						
							if (prevDrawCommand.drawCommand.sortKey.sortKey != currDrawCommand.drawCommand.sortKey.sortKey)
							{
								instancingRanges.emplace_back(index, 1);
							}
							else
							{
								instancingRanges.back().count++;
							}
						}
					}
				});
			}
		}

		// Run the tasks.
		taskGraph.ExecuteAndWait();

		// Setup data
		uint32_t numDrawCommands = 0;
		for (const MeshDrawCommandBucket& drawCommandBucket : m_meshDrawCommandBuckets)
		{
			numDrawCommands += static_cast<uint32_t>(drawCommandBucket.drawCommands.size());
		}

		const size_t primitiveIndexDataSize = sizeof(uint32_t) * numDrawCommands;
		const size_t meshDataSize = sizeof(DebugMeshData) * numDrawCommands;

		uint32_t* primitiveIndexDataPtr = reinterpret_cast<uint32_t*>(renderGraph.AllocData(primitiveIndexDataSize));
		DebugMeshData* debugMeshDataPtr = reinterpret_cast<DebugMeshData*>(renderGraph.AllocData(meshDataSize));

		uint32_t elementIndex = 0;
		for (const MeshDrawCommandBucket& drawCommandBucket : m_meshDrawCommandBuckets)
		{
			for (const MeshDrawCommandBucket::MeshDrawCommandInfo& drawCommandInfo : drawCommandBucket.drawCommands)
			{
				primitiveIndexDataPtr[elementIndex] = elementIndex;
				debugMeshDataPtr[elementIndex].position = drawCommandInfo.transform.translation;
				debugMeshDataPtr[elementIndex].scale = drawCommandInfo.transform.scale;
				debugMeshDataPtr[elementIndex].rotation = drawCommandInfo.transform.rotation;
				debugMeshDataPtr[elementIndex].userData = drawCommandInfo.userData;
			}
		}

		// Create buffers
		RGBufferDesc primitiveIndexBufferDesc{};
		primitiveIndexBufferDesc.count = numDrawCommands;
		primitiveIndexBufferDesc.elementSize = sizeof(uint32_t);
		primitiveIndexBufferDesc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::StorageBuffer;
		primitiveIndexBufferDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		primitiveIndexBufferDesc.debugName = "Debug.PrimitiveIndexVertexBuffer";

		m_primitiveIndexDataBuffer = renderGraph.CreateBuffer(primitiveIndexBufferDesc);

		RGBufferDesc debugMeshDataBufferDesc{};
		debugMeshDataBufferDesc.count = numDrawCommands;
		debugMeshDataBufferDesc.elementSize = sizeof(DebugMeshData);
		debugMeshDataBufferDesc.usage = RHI::BufferUsage::StorageBuffer;
		debugMeshDataBufferDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
		debugMeshDataBufferDesc.debugName = "Debug.DebugMeshDataBuffer";

		m_debugMeshDataBuffer = renderGraph.CreateBuffer(debugMeshDataBufferDesc);

		AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_primitiveIndexDataBuffer), primitiveIndexDataPtr, primitiveIndexDataSize);
		AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_debugMeshDataBuffer), debugMeshDataPtr, meshDataSize);
	}

	void DebugMeshRenderer::Reset()
	{
		m_meshDrawCommandBuckets.clear();
		m_hashKeyToBucketIndex.clear();
		m_primitiveIndexDataBuffer = nullptr;
		m_debugMeshDataBuffer = nullptr;
	}

	void DebugMeshRenderer::BuildMeshDrawCommand(Ref<Mesh> mesh,
		Ref<RenderMaterial> renderMaterial, 
		const TQS& transform, 
		uint32_t userData, 
		RHI::RenderPipelineCreateInfo pipelineInfo, 
		RefPtr<RHI::Shader> vertexShader, 
		RefPtr<RHI::Shader> pixelShader)
	{
		VT_ENSURE_MSG(vertexShader && pixelShader, "Valid shaders must be supplied!");
		pipelineInfo.shaders = { vertexShader, pixelShader };

		RefPtr<RHI::RenderPipeline> renderPipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

		RHI::VertexBufferVector vertexBuffers;
		vertexBuffers.emplace_back(mesh->GetVertexPositionsBuffer());
		vertexBuffers.emplace_back(mesh->GetVertexMaterialBuffer());
		vertexBuffers.emplace_back(mesh->GetVertexAnimationInfoBuffer());

		MeshDrawCommandSortKey sortKey = GetSortKeyFromMaterial(vertexShader, pixelShader, *renderMaterial);

		for (const SubMesh& subMesh : mesh->GetSubMeshes())
		{
			MeshDrawCommandHashKey bucketHashKey{};
			bucketHashKey.hashKeyContents.vertexBufferHash = mesh->GetVertexPositionsBuffer().GetHash();
			bucketHashKey.hashKeyContents.indexBufferHash = mesh->GetIndexBuffer().GetHash();
			bucketHashKey.hashKeyContents.subMeshHash = subMesh.GetHash();

			TQS subMeshTransform = { subMesh.transform.position, subMesh.transform.rotation, subMesh.transform.scale };

			MeshDrawCommandBucket& drawCommandBucket = GetOrCreateBucket(bucketHashKey);
			MeshDrawCommandBucket::MeshDrawCommandInfo& newDrawCommand = drawCommandBucket.drawCommands.emplace_back();
			newDrawCommand.drawCommand.renderPipeline = renderPipeline;
			newDrawCommand.drawCommand.vertexBuffers = vertexBuffers;
			newDrawCommand.drawCommand.indexBuffer = mesh->GetIndexBuffer();
			newDrawCommand.drawCommand.renderPrimitive = nullptr;
			newDrawCommand.drawCommand.drawCommand.indexCount = subMesh.indexCount;
			newDrawCommand.drawCommand.drawCommand.instanceCount = 1;
			newDrawCommand.drawCommand.drawCommand.firstIndex = subMesh.indexStartOffset;
			newDrawCommand.drawCommand.drawCommand.vertexOffset = subMesh.vertexStartOffset;
			newDrawCommand.drawCommand.drawCommand.firstInstance = 0;
			newDrawCommand.drawCommand.sortKey = sortKey;
			newDrawCommand.transform = TQS::Combine(transform, subMeshTransform);
			newDrawCommand.userData = userData;
			newDrawCommand.material = renderMaterial;
		}
	}

	DebugMeshRenderer::MeshDrawCommandBucket& DebugMeshRenderer::GetOrCreateBucket(MeshDrawCommandHashKey hashKey)
	{
		auto it = m_hashKeyToBucketIndex.find(hashKey);
		if (it != m_hashKeyToBucketIndex.end())
		{
			return m_meshDrawCommandBuckets.at(it->second);
		}

		MeshDrawCommandBucket& newBucket = m_meshDrawCommandBuckets.emplace_back();
		m_hashKeyToBucketIndex[hashKey] = m_meshDrawCommandBuckets.size() - 1;

		return newBucket;
	}

	MeshDrawCommandSortKey DebugMeshRenderer::GetSortKeyFromMaterial(RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, RenderMaterial& renderMaterial)
	{
		MeshDrawCommandSortKey sortKey;
		sortKey.sortKeyContents.vertexShaderHash = vertexShader->GetHash();
		sortKey.sortKeyContents.pixelShaderHash = pixelShader->GetHash();
		sortKey.sortKeyContents.permutationHash = GetMaterialPermutationHash(renderMaterial);

		return sortKey;
	}

	size_t DebugMeshRenderer::GetMaterialPermutationHash(RenderMaterial& renderMaterial)
	{
		uint64_t result = 0;
		result = Math::HashCombine(std::hash<std::underlying_type_t<MaterialBlendMode>>()(std::to_underlying(renderMaterial.GetMaterialBlendMode())), std::hash<bool>()(renderMaterial.GetIsDoubleSided()));

		return result;
	}

	DebugMeshRendererRegistry::~DebugMeshRendererRegistry()
	{
		for (DestructorHelper& destructorHelper : m_debugMeshRendererDestructors)
		{
			destructorHelper.Destroy();
		}
	}

	void DebugMeshRendererRegistry::AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, uint32_t userData)
	{
		for (DebugMeshRendererContainer& container : m_debugMeshRenderers)
		{
			if (container.debugMeshRenderer->ShouldIncludeDraw(*renderMaterial))
			{
				container.debugMeshRenderer->AddMeshDraw(mesh, renderMaterial, transform, userData);
			}
		}
	}

	void DebugMeshRendererRegistry::PrepareMeshesForRendering(RenderGraph& renderGraph)
	{
		for (DebugMeshRendererContainer& container : m_debugMeshRenderers)
		{
			container.debugMeshRenderer->PrepareMeshesForRendering(renderGraph);
		}
	}

	void DebugMeshRendererRegistry::Reset()
	{
		for (DebugMeshRendererContainer& container : m_debugMeshRenderers)
		{
			container.debugMeshRenderer->Reset();
		}
	}
}
