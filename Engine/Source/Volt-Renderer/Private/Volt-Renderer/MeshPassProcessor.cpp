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

	MeshPassProcessorRegistry::MeshPassProcessorRegistry(RenderScene* renderScene)
		: m_renderScene(renderScene)
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

	void MeshPassProcessorRegistry::RemoveRenderPrimitive(const RenderPrimitiveData& renderPrimitive)
	{
		for (MeshPassProcessor* meshPassProcessor : m_meshPassProcessors)
		{
			meshPassProcessor->RemoveRenderPrimitive(renderPrimitive);
		}
	}

	void MeshPassProcessorRegistry::AddPrimitivesToMeshPassProcessor(MeshPassProcessor* meshPassProcessor)
	{
		for (const RenderPrimitiveData& renderPrimitive : (*m_renderScene))
		{
			meshPassProcessor->AddRenderPrimitive(renderPrimitive);
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
		for (const auto& meshDrawCommandBucket : m_meshDrawCommandBuckets)
		{
			primitiveIndices.reserve(primitiveIndices.size() + meshDrawCommandBucket.drawCommands.size());

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

			AddMappedBufferUpload(renderGraph, renderGraph.CreateUAV(m_primitiveIndexVertexBuffer), primitiveIndices.data(), primitiveIndices.byte_size());
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

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();
		RefPtr<RHI::StorageBuffer> primitiveIndexVertexBuffer = m_primitiveIndexVertexBuffer->GetRHIResource()->GetRHIBuffer();

		RHI::VertexBufferVector primitiveIndexVertexBufferVector;
		primitiveIndexVertexBufferVector.resize(1);
		primitiveIndexVertexBufferVector[0].buffer = primitiveIndexVertexBuffer;

		uint64_t primitiveOffset = 0;
		for (const MeshDrawCommandBucket& drawCommandBucket : m_meshDrawCommandBuckets)
		{
			VT_PROFILE_SCOPE("DrawCommandBucket");

			for (const MeshDrawCommandBucket::InstancingRange& instancingRange : drawCommandBucket.instancingRanges)
			{
				const MeshDrawCommand& firstDrawComamnd = drawCommandBucket.drawCommands.at(instancingRange.offset);

				ArrayView<RHI::ShaderParameterMap> shaderParametersMaps = firstDrawComamnd.renderPipeline->GetShaderParameterMaps();
				InlineVector<RenderContext::PerStageShaderParameters, 8> perShaderStageParameters = renderContext.SetupPipelineData(firstDrawComamnd.renderPipeline);

				RHI::ShaderBindingMap shaderBindings = RHI::ShaderBindingMap::InitializeFromPipeline(firstDrawComamnd.renderPipeline);
				batchedShaderParameters.BindShaderBindings(shaderParametersMaps, shaderBindings);
				batchedShaderParameters.PopulateShaderParameterUniformBuffers(shaderParametersMaps, perShaderStageParameters);

				for (auto& shaderParameters : perShaderStageParameters)
				{
					shaderBindings.SetUniformBufferWithSizeAndOffset(shaderParameters.shaderStage, RHI::Globals::SHADER_GLOBALS_BINDING, shaderParameters.uniformBufferSRV->GetRHIView(), shaderParameters.size, shaderParameters.offset);
				}

				VT_ENSURE_MSG(firstDrawComamnd.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.layout.IsValid(), "Mesh pass processors must have a per instance layout!");

				const uint32_t perInstanceBindingIndex = firstDrawComamnd.renderPipeline->GetVertexBufferLayout().perInstanceVertexBuffer.bindingIndex;

				primitiveIndexVertexBufferVector[0].offset = (primitiveOffset + instancingRange.offset) * sizeof(uint32_t);

				commandBuffer->BindPipeline(firstDrawComamnd.renderPipeline);
				commandBuffer->BindShaderBindings(shaderBindings);
				commandBuffer->BindVertexBuffers(firstDrawComamnd.vertexBuffers, 0);
				commandBuffer->BindVertexBuffers(primitiveIndexVertexBufferVector, perInstanceBindingIndex);
				commandBuffer->BindIndexBuffer(firstDrawComamnd.indexBuffer);
				commandBuffer->DrawIndexed(firstDrawComamnd.drawCommand.indexCount, instancingRange.count, firstDrawComamnd.drawCommand.firstIndex, firstDrawComamnd.drawCommand.vertexOffset, firstDrawComamnd.drawCommand.firstInstance);
			}

			primitiveOffset += drawCommandBucket.drawCommands.size();
		}
	}

	void MeshPassProcessor::BuildMeshDrawCommand(const RenderPrimitiveData& renderPrimitive, RHI::RenderPipelineCreateInfo pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader)
	{
		VT_ENSURE_MSG(vertexShader && pixelShader, "Valid shaders must be supplied!");

		pipelineInfo.shaders = { vertexShader, pixelShader };

		RefPtr<RHI::RenderPipeline> renderPipeline = PipelineStateCache::GetRenderPipeline(pipelineInfo);

		const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

		const MeshDrawCommandHashKey hashKey = GetHashKeyFromRenderPrimitive(renderPrimitive);

		MeshDrawCommandBucket& drawCommandBucket = GetOrCreateBucket(hashKey);

		MeshDrawCommand& newDrawCommand = drawCommandBucket.drawCommands.emplace_back();
		newDrawCommand.renderPipeline = renderPipeline;
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexPositionsBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexMaterialBuffer());
		newDrawCommand.vertexBuffers.emplace_back(renderPrimitive.mesh->GetVertexAnimationInfoBuffer());
		newDrawCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer();
		newDrawCommand.primitiveIndex = renderPrimitive.primitiveIndex;
		newDrawCommand.renderPrimitiveID = renderPrimitive.id;
		newDrawCommand.drawCommand.indexCount = subMesh.indexCount;
		newDrawCommand.drawCommand.instanceCount = 1;
		newDrawCommand.drawCommand.firstIndex = subMesh.indexStartOffset;
		newDrawCommand.drawCommand.vertexOffset = subMesh.vertexStartOffset;
		newDrawCommand.drawCommand.firstInstance = 0;

		newDrawCommand.sortKey.sortKeyContents.pixelShaderHash = pixelShader->GetHash();
		newDrawCommand.sortKey.sortKeyContents.vertexShaderHash = vertexShader->GetHash();

		MarkBucketDirty(hashKey);
   	}

	void MeshPassProcessor::RemoveMeshDrawCommand(const RenderPrimitiveData& renderPrimitive)
	{
		const MeshDrawCommandHashKey hashKey = GetHashKeyFromRenderPrimitive(renderPrimitive);

		MeshDrawCommandBucket* drawCommandBucket = TryGetBucket(hashKey);

		if (drawCommandBucket != nullptr)
		{
			Vector<MeshDrawCommand>& drawCommands = drawCommandBucket->drawCommands;

			for (int32_t drawCommandIndex = static_cast<int32_t>(drawCommands.size()) - 1; drawCommandIndex >= 0; --drawCommandIndex)
			{
				if (drawCommands[drawCommandIndex].renderPrimitiveID == renderPrimitive.id)
				{
					drawCommands.erase(drawCommands.begin() + drawCommandIndex);
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
								instancingOffsets.emplace_back(index);
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

	MeshDrawCommandHashKey MeshPassProcessor::GetHashKeyFromRenderPrimitive(const RenderPrimitiveData& renderPrimitive)
	{
		const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

		MeshDrawCommandHashKey hashKey;
		hashKey.hashKeyContents.vertexBufferHash = renderPrimitive.mesh->GetVertexPositionsBuffer().GetHash();
		hashKey.hashKeyContents.indexBufferHash = renderPrimitive.mesh->GetIndexBuffer().GetHash();
		hashKey.hashKeyContents.subMeshHash = subMesh.GetHash();


		return hashKey;
	}
}
