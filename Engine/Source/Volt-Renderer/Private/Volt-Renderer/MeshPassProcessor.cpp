#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessor.h"
#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/RenderScene.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <JobSystem/JobSystem.h>
#include <JobSystem/TaskGraph.h>

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/RenderGraph/RenderGraph.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>
#include <RenderCore/RenderGraph/RenderGraphUtils.h>

#include <RHIModule/Globals.h>
#include <RHIModule/RHICapabilities.h>

namespace Volt
{
	static ConsoleVariable<int32_t> s_forceImmediateSorting(
		"r.MeshPassProcessor.ForceImmediateSorting",
		1,
		"Whether or not force immediate sorting of mesh draw commands.");

	static ConsoleVariable<int32_t> s_forceSingleThreadedRecording(
		"r.MeshPassProcessor.ForceSingleThreadedRecording",
		0,
		"Whether or not force single threaded recording of mesh draw commands.");

	MeshPassProcessorRegistry::MeshPassProcessorRegistry(RenderScene* renderScene)
		: m_renderScene(renderScene)
	{
	}

	MeshPassProcessorRegistry::~MeshPassProcessorRegistry()
	{
		for (DestructorHelper& destructorHelper : m_meshPassDestructors)
		{
			destructorHelper.Destroy();
		}
	}

	void MeshPassProcessorRegistry::AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		for (MeshPassProcessor* meshPassProcessor : m_meshPassProcessors)
		{
			if (meshPassProcessor->ShouldIncludePrimitive(renderPrimitive))
			{
				meshPassProcessor->AddRenderPrimitive(renderPrimitive);
			}
		}
	}

	void MeshPassProcessorRegistry::RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		for (MeshPassProcessor* meshPassProcessor : m_meshPassProcessors)
		{
			if (meshPassProcessor->ShouldIncludePrimitive(renderPrimitive))
			{
				meshPassProcessor->RemoveRenderPrimitive(renderPrimitive);
			}
		}
	}

	void MeshPassProcessorRegistry::AddPrimitivesToMeshPassProcessor(MeshPassProcessor* meshPassProcessor)
	{
		Vector<RenderPrimitiveData*> renderPrimitives = m_renderScene->GetRenderPrimitives();

		for (const RenderPrimitiveData* renderPrimitive : renderPrimitives)
		{
			if (meshPassProcessor->ShouldIncludePrimitive(renderPrimitive))
			{
				meshPassProcessor->AddRenderPrimitive(renderPrimitive);
			}
		}
	}

	MeshPassProcessor::MeshPassProcessor()
	{
	}

	MeshPassProcessor::~MeshPassProcessor()
	{
		if (m_sortTaskCounter)
		{
			JobSystem::DestroyCounter(m_sortTaskCounter);
		}
	}

	void MeshPassProcessor::PrepareRenderCommands(RenderGraph& renderGraph)
	{
		VT_PROFILE_FUNCTION();

		JobSystem::WaitForAndDestroyCounter(m_sortTaskCounter);

		uint32_t numPrimitivesToRender = 0;

		Vector<uint32_t> primitiveIndices;

		m_meshDrawCommandBucketPrimitiveOffsets.clear();

		for (const auto& meshDrawCommandBucket : m_meshDrawCommandBuckets)
		{
			primitiveIndices.reserve(primitiveIndices.size() + meshDrawCommandBucket.drawCommands.size());
			m_meshDrawCommandBucketPrimitiveOffsets.emplace_back(numPrimitivesToRender);

			for (const auto& drawCommand : meshDrawCommandBucket.drawCommands)
			{
				primitiveIndices.emplace_back(drawCommand.primitiveIndex);
			}

			numPrimitivesToRender += static_cast<uint32_t>(meshDrawCommandBucket.drawCommands.size());
		}

		if (numPrimitivesToRender > 0)
		{
			RGBufferDesc bufferDesc{};
			bufferDesc.count = numPrimitivesToRender;
			bufferDesc.elementSize = sizeof(uint32_t);
			bufferDesc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::StorageBuffer;
			bufferDesc.memoryUsage = RHI::MemoryUsage::CPUToGPU;
			bufferDesc.debugName = "PrimitiveIndexVertexBuffer";

			m_primitiveIndexVertexBuffer = renderGraph.CreateBuffer(bufferDesc);

			m_primitiveIndexVertexBuffer->AddRef();

			AddMappedBufferUploadCopyData(renderGraph, renderGraph.CreateUAV(m_primitiveIndexVertexBuffer), primitiveIndices.data(), primitiveIndices.byte_size());
		}
		else
		{
			m_primitiveIndexVertexBuffer = nullptr;
		}
	}

	void MeshPassProcessor::ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		JobSystem::WaitForAndDestroyCounter(m_sortTaskCounter);

		if (!m_primitiveIndexVertexBuffer)
		{
			return;
		}

		RefPtr<RHI::CommandBuffer> mainCommandBuffer = renderContext.GetRHICommandBuffer();
		RefPtr<RHI::StorageBuffer> primitiveIndexVertexBuffer = m_primitiveIndexVertexBuffer->GetRHIResource()->GetRHIBuffer();

		RHI::RenderingAttachmentDeclaration renderingAttachmentDeclaration;
		renderContext.FillRenderingAttachmentDeclaration(renderingAttachmentDeclaration);

		constexpr uint32_t NumMaxBucketsPerCommandBuffer = 128;
		//constexpr uint32_t NumMaxCommandBuffers = 512;

		const RenderingInfo& activeRenderingInfo = renderContext.GetActiveRenderingInfo();

		auto recordBucketRange = [&]<bool IsSecondary>(uint32_t startIndex, uint32_t num, uint32_t primitiveOffset, RefPtr<RHI::CommandBuffer> commandBuffer, std::bool_constant<IsSecondary>)
		{
			VT_PROFILE_SCOPE("Record DrawCommandBucket Range");

			if constexpr (IsSecondary)
			{
				commandBuffer->Begin(true);
				commandBuffer->SetScissors({ activeRenderingInfo.scissor });
				commandBuffer->SetViewports({ activeRenderingInfo.viewport });
			}

			RHI::VertexBufferVector primitiveIndexVertexBufferVector;
			primitiveIndexVertexBufferVector.resize(1);
			primitiveIndexVertexBufferVector[0].buffer = primitiveIndexVertexBuffer;

			RawPtr<RHI::RenderPipeline> prevRenderPipeline;

			for (uint32_t i = startIndex; i < startIndex + num; ++i)
			{
				VT_PROFILE_SCOPE("Draw Command Bucket");

				MeshDrawCommandBucket& currentBucket = m_meshDrawCommandBuckets[i];

				 for (const MeshDrawCommandBucket::InstancingRange& instancingRange : currentBucket.instancingRanges)
				 {
					 VT_PROFILE_SCOPE("Instancing Range");

					 MeshDrawCommand& firstDrawCommand = currentBucket.drawCommands[instancingRange.offset];
					 firstDrawCommand.renderPipelineInfo.colorAttachmentFormats = renderingAttachmentDeclaration.colorAttachmentFormats;
					 firstDrawCommand.renderPipelineInfo.depthAttachmentFormat = renderingAttachmentDeclaration.depthAttachmentFormat;

					 RefPtr<RHI::RenderPipeline> drawCommandPipeline = PipelineStateCache::GetRenderPipeline(firstDrawCommand.renderPipelineInfo);

					 VT_ENSURE_MSG(drawCommandPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.layout.IsValid(), "Mesh pass processors must have a per instance layout!");
					 const uint32_t perInstanceBindingIndex = drawCommandPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.bindingIndex;

					 primitiveIndexVertexBufferVector[0].offset = (primitiveOffset + instancingRange.offset) * sizeof(uint32_t);

					 // We don't need to rebind the same pipeline.
					 const bool shouldBindPipeline = (prevRenderPipeline == nullptr || prevRenderPipeline != drawCommandPipeline);
					 if (shouldBindPipeline)
					 {
						 prevRenderPipeline = drawCommandPipeline;
						 commandBuffer->BindPipeline(drawCommandPipeline);

						 ArrayView<RHI::ShaderParameterMap> shaderParametersMaps = drawCommandPipeline->GetShaderParameterMaps();
						 InlineVector<RenderContext::PerStageShaderParameters, 8> perShaderStageParameters = renderContext.SetupPipelineData(drawCommandPipeline);

						 RHI::ShaderBindingMap shaderBindings = RHI::ShaderBindingMap::InitializeFromPipeline(drawCommandPipeline);
						 batchedShaderParameters.BindShaderBindings(shaderParametersMaps, shaderBindings);
						 batchedShaderParameters.PopulateShaderParameterUniformBuffers(shaderParametersMaps, perShaderStageParameters);

						 for (auto& shaderParameters : perShaderStageParameters)
						 {
							 shaderBindings.SetUniformBufferWithSizeAndOffset(shaderParameters.shaderStage, RHI::Globals::SHADER_GLOBALS_BINDING, shaderParameters.uniformBufferSRV->GetRHIView(), shaderParameters.size, shaderParameters.offset);
						 }

						 firstDrawCommand.renderPrimitive->material->BindToShaderBindingMap(shaderBindings, drawCommandPipeline);

						 commandBuffer->BindShaderBindings(shaderBindings);
					 }

					 commandBuffer->BindVertexBuffers(firstDrawCommand.vertexBuffers, 0);
					 commandBuffer->BindVertexBuffers(primitiveIndexVertexBufferVector, perInstanceBindingIndex);
					 commandBuffer->BindIndexBuffer(firstDrawCommand.indexBuffer);

					 if (drawCommandPipeline->HasInlineParameters())
					 {
						 MaterialShader::InlineParameterBlock inlineParameterBlock = GetMaterialInlineParameterBlock(firstDrawCommand.renderPrimitive);
						 commandBuffer->PushInlineParameters(&inlineParameterBlock, sizeof(MaterialShader::InlineParameterBlock), 0, RHI::ShaderStage::Pixel);
					 }

					 commandBuffer->DrawIndexed(
						 firstDrawCommand.drawCommand.indexCount,
						 instancingRange.count,
						 firstDrawCommand.drawCommand.firstIndex,
						 firstDrawCommand.drawCommand.vertexOffset,
						 firstDrawCommand.drawCommand.firstInstance);
				 }

				 primitiveOffset += static_cast<uint32_t>(currentBucket.drawCommands.size());
			}

			if constexpr (IsSecondary)
			{
				commandBuffer->End();
			}
		};

		const uint32_t numDrawBuckets = static_cast<uint32_t>(m_meshDrawCommandBuckets.size());

		// Record into the main command buffer
		if (m_meshDrawCommandBuckets.size() < NumMaxBucketsPerCommandBuffer)
		{
			recordBucketRange(0, numDrawBuckets, 0, mainCommandBuffer, std::bool_constant<false>{});
		}
		// Dispatch jobs to record the secondary command buffers.
		else
		{
			struct RecordingRange
			{
				uint32_t offset;
				uint32_t num;
			};

			const uint32_t numCommandBuffers = Math::DivideRoundUp(numDrawBuckets, NumMaxBucketsPerCommandBuffer);

			Vector<RefPtr<RHI::CommandBuffer>> commandBuffers;
			Vector<RecordingRange> commandBufferRanges;

			commandBuffers.resize(numCommandBuffers);
			commandBufferRanges.resize(numCommandBuffers);

			uint32_t currentOffset = 0;
			for (uint32_t i = 0; i < numCommandBuffers; ++i)
			{
				commandBuffers[i] = RHI::CommandBuffer::CreateSecondary(&renderingAttachmentDeclaration);

				commandBufferRanges[i].offset = currentOffset;
				commandBufferRanges[i].num = (currentOffset + NumMaxBucketsPerCommandBuffer) < numDrawBuckets ? NumMaxBucketsPerCommandBuffer : (numDrawBuckets - currentOffset);
			
				currentOffset += commandBufferRanges[i].num;
			}

			// Job system recording
			if (!s_forceSingleThreadedRecording.GetValue())
			{
				TaskGraph taskGraph{ ExecutionPriority::Render };

				for (size_t i = 0; i < commandBuffers.size(); ++i)
				{
					taskGraph.AddTask("Record Mesh Pass", [i, this, &commandBufferRanges, &recordBucketRange, &commandBuffers]()
					{
						const uint32_t primitiveOffset = m_meshDrawCommandBucketPrimitiveOffsets.at(commandBufferRanges[i].offset);
						recordBucketRange(commandBufferRanges[i].offset, commandBufferRanges[i].num, primitiveOffset, commandBuffers[i], std::bool_constant<true>{});
					});
				}

				taskGraph.ExecuteAndWait();
			}
			// Pass execution thread recording (might be in a worker thread)
			else
			{
				for (size_t i = 0; i < commandBuffers.size(); ++i)
				{
					const uint32_t primitiveOffset = m_meshDrawCommandBucketPrimitiveOffsets.at(commandBufferRanges[i].offset);
					recordBucketRange(commandBufferRanges[i].offset, commandBufferRanges[i].num, primitiveOffset, commandBuffers[i], std::bool_constant<true>{});
				}
			}

			mainCommandBuffer->ExecuteSecondaryCommandBuffers(commandBuffers);
		}
	}

	void MeshPassProcessor::BuildMeshDrawCommand(const RenderPrimitiveData* renderPrimitive, RHI::RenderPipelineCreateInfo pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader)
	{
		VT_ENSURE_MSG(vertexShader && pixelShader, "Valid shaders must be supplied!");

		pipelineInfo.shaders = { vertexShader, pixelShader };

		const SubMesh& subMesh = renderPrimitive->mesh->GetSubMeshes().at(renderPrimitive->subMeshIndex);

		const MeshDrawCommandHashKey hashKey = GetHashKeyFromRenderPrimitive(renderPrimitive);

		MeshDrawCommandBucket& drawCommandBucket = GetOrCreateBucket(hashKey);

		MeshDrawCommand& newDrawCommand = drawCommandBucket.drawCommands.emplace_back();
		newDrawCommand.renderPipelineInfo = std::move(pipelineInfo);
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive->mesh->GetVertexPositionsBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive->mesh->GetVertexMaterialBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive->mesh->GetVertexAnimationInfoBuffer());
		newDrawCommand.indexBuffer = renderPrimitive->mesh->GetIndexBuffer();
		newDrawCommand.primitiveIndex = renderPrimitive->primitiveIndex;
		newDrawCommand.renderPrimitive = renderPrimitive;
		newDrawCommand.drawCommand.indexCount = subMesh.indexCount;
		newDrawCommand.drawCommand.instanceCount = 1;
		newDrawCommand.drawCommand.firstIndex = subMesh.indexStartOffset;
		newDrawCommand.drawCommand.vertexOffset = subMesh.vertexStartOffset;
		newDrawCommand.drawCommand.firstInstance = 0;

		newDrawCommand.sortKey = GetSortKeyFromRenderPrimitive(renderPrimitive, pixelShader, vertexShader);

		MarkBucketDirty(hashKey);
   	}

	void MeshPassProcessor::RemoveMeshDrawCommand(const RenderPrimitiveData* renderPrimitive)
	{
		const MeshDrawCommandHashKey hashKey = GetHashKeyFromRenderPrimitive(renderPrimitive);

		MeshDrawCommandBucket* drawCommandBucket = TryGetBucket(hashKey);

		if (drawCommandBucket != nullptr)
		{
			Vector<MeshDrawCommand>& drawCommands = drawCommandBucket->drawCommands;

			for (int32_t drawCommandIndex = static_cast<int32_t>(drawCommands.size()) - 1; drawCommandIndex >= 0; --drawCommandIndex)
			{
				if (drawCommands[drawCommandIndex].renderPrimitive->id == renderPrimitive->id)
				{
					drawCommands.erase_unsorted(drawCommands.begin() + drawCommandIndex);
					MarkBucketDirty(hashKey);
					break;
				}
			}
		}
	}

	MeshPassProcessor::MeshDrawCommandBucket& MeshPassProcessor::GetOrCreateBucket(MeshDrawCommandHashKey hashKey)
	{
		auto it = m_hashKeyToBucketIndex.find(hashKey);
		if (it != m_hashKeyToBucketIndex.end())
		{
			return m_meshDrawCommandBuckets[it->second];
		}

		const size_t newIndex = m_meshDrawCommandBuckets.size();
		auto& bucket = m_meshDrawCommandBuckets.emplace_back();

		m_hashKeyToBucketIndex[hashKey] = newIndex;

		return bucket;
	}

	MeshPassProcessor::MeshDrawCommandBucket* MeshPassProcessor::TryGetBucket(MeshDrawCommandHashKey hashKey)
	{
		auto it = m_hashKeyToBucketIndex.find(hashKey);
		if (it != m_hashKeyToBucketIndex.end())
		{
			return &m_meshDrawCommandBuckets[it->second];
		}

		return nullptr;
	}

	void MeshPassProcessor::MarkBucketDirty(MeshDrawCommandHashKey hashKey)
	{
		MeshDrawCommandBucket* bucket = TryGetBucket(hashKey);
		if (bucket != nullptr)
		{
			if (!bucket->isDirty)
			{
				bucket->isDirty = true;

				auto findInstancingOffsetsFunc = [bucket]()
				{
					auto& drawCommands = bucket->drawCommands;
					auto& instancingOffsets = bucket->instancingRanges;

					instancingOffsets.clear();

					for (size_t index = 0; index < drawCommands.size(); ++index)
					{
						if (index == 0)
						{
							instancingOffsets.emplace_back(0, 1);
						}
						else
						{
							const MeshDrawCommand& prevDrawCommand = drawCommands.at(index - 1);
							const MeshDrawCommand& currDrawCommand = drawCommands.at(index);

							if (prevDrawCommand.sortKey.sortKey != currDrawCommand.sortKey.sortKey)
							{
								instancingOffsets.emplace_back(index, 1);
							}
							else
							{
								instancingOffsets.back().count++;
							}
						}
					}
				};

				auto sortDrawCommandsFunc = [bucket]() 
				{
					auto& drawCommands = bucket->drawCommands;
					std::sort(drawCommands.begin(), drawCommands.end(), [](const MeshDrawCommand& lhs, const MeshDrawCommand& rhs)
					{
						return lhs.sortKey.sortKey < rhs.sortKey.sortKey;
					});

					bucket->isDirty = false;
				};

				if (s_forceImmediateSorting.GetValue())
				{
					sortDrawCommandsFunc();
					findInstancingOffsetsFunc();
				}
				else
				{
					// Make sure previous tasks have finished.
					JobSystem::WaitForAndDestroyCounter(m_sortTaskCounter);

					TaskGraph taskGraph{ ExecutionPriority::Render, 2 };
					TaskGraph::Task* sortTask = taskGraph.AddTask("MeshPassProcessor::SortMeshDrawCommandBucket", std::move(sortDrawCommandsFunc));
					taskGraph.AddTaskWithDependencies("MeshPassProcessor::FindInstancingOffsets", { sortTask }, std::move(findInstancingOffsetsFunc));

					m_sortTaskCounter = taskGraph.ExecuteAndExtractCounter();
				}
			}
		}
	}

	MeshDrawCommandHashKey MeshPassProcessor::GetHashKeyFromRenderPrimitive(const RenderPrimitiveData* renderPrimitive)
	{
		const SubMesh& subMesh = renderPrimitive->mesh->GetSubMeshes().at(renderPrimitive->subMeshIndex);

		MeshDrawCommandHashKey hashKey;
		hashKey.hashKeyContents.vertexBufferHash = renderPrimitive->mesh->GetVertexPositionsBuffer().GetHash();
		hashKey.hashKeyContents.indexBufferHash = renderPrimitive->mesh->GetIndexBuffer().GetHash();
		hashKey.hashKeyContents.subMeshHash = subMesh.GetHash();

		return hashKey;
	}

	MeshDrawCommandSortKey MeshPassProcessor::GetSortKeyFromRenderPrimitive(const RenderPrimitiveData* renderPrimitive, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader)
	{
		MeshDrawCommandSortKey sortKey;
		sortKey.sortKeyContents.vertexShaderHash = vertexShader->GetHash();
		sortKey.sortKeyContents.pixelShaderHash = pixelShader->GetHash();
		sortKey.sortKeyContents.permutationHash = GetMaterialPermutationHash(renderPrimitive->material);
	
		return sortKey;
	}

	uint64_t MeshPassProcessor::GetMaterialPermutationHash(Weak<RenderMaterial> renderMaterial)
	{
		uint64_t result = 0;
		result = Math::HashCombine(std::hash<std::underlying_type_t<MaterialBlendMode>>()(std::to_underlying(renderMaterial->GetMaterialBlendMode())), std::hash<bool>()(renderMaterial->GetIsDoubleSided()));

		return result;
	}

	MaterialShader::InlineParameterBlock MeshPassProcessor::GetMaterialInlineParameterBlock(const RenderPrimitiveData* renderPrimitive) const
	{
		MaterialShader::InlineParameterBlock parameterBlock;
		parameterBlock.materialBlendMode = std::to_underlying(renderPrimitive->material->GetMaterialBlendMode());
		parameterBlock.isDoubleSided = renderPrimitive->material->GetIsDoubleSided();

		return parameterBlock;
	}
}
