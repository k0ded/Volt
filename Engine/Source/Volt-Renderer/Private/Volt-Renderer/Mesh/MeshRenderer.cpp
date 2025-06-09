#include "vrpch.h"

#include "Volt-Renderer/Mesh/MeshRenderer.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RenderPrimitiveData.h"

#include <RenderCore/RenderGraph/RenderContext.h>
#include <RenderCore/Shader/ShaderMap.h>
#include <RenderCore/Shader/PipelineStateCache.h>
#include <RenderCore/DescriptorTableCache.h>
#include <RenderCore/Shader/BatchedShaderParameters.h>

#include <CoreUtilities/EnumUtils.h>

namespace Volt
{
	void MeshRenderer::BuildRenderCommands(Ref<RenderScene> renderScene, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		VT_PROFILE_FUNCTION();

		m_renderCommands.clear();
		m_meshBatches.clear();

		struct RenderCommandExt : public RenderCommand
		{
			size_t vertexIndexBufferHash;
			size_t subMeshHash;
			size_t renderPipelineHash;
		
			MeshBatch::VertexBufferVector vertexBuffers;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
		};

		Vector<RenderCommandExt> renderCommandExts;
		renderCommandExts.reserve(renderScene->GetRenderObjectCount());

		const RHI::ShaderInfo& vertexShaderInfo = vertexShader->GetShaderInfo();

		// First get all commands, their info and the required hashes for sorting.
		for (const RenderPrimitiveData& renderPrimitive : *renderScene)
		{
			const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

			auto& newCommand = renderCommandExts.emplace_back();
			newCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer()->GetResource();
			newCommand.indexCount = subMesh.indexCount;
			newCommand.firstIndex = subMesh.indexStartOffset;
			newCommand.vertexOffset = subMesh.vertexStartOffset;
			newCommand.primitiveIndex = renderScene->GetPrimitiveIndexFromID(renderPrimitive.id);

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
			renderPipelineInfo.shaders = { vertexShader, pixelShader };
			newCommand.renderPipeline = PipelineStateCache::GetRenderPipeline(renderPipelineInfo);

			newCommand.vertexIndexBufferHash = newCommand.indexBuffer.GetHash();
			
			for (const auto& vertexBuffer : newCommand.vertexBuffers)
			{
				newCommand.vertexIndexBufferHash = Math::HashCombine(newCommand.vertexIndexBufferHash, vertexBuffer.GetHash());
			}

			newCommand.subMeshHash = subMesh.GetHash();
			newCommand.renderPipelineHash = newCommand.renderPipeline->GetHash();
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

		MeshBatch* currentMeshBatch = nullptr;

		for (size_t i = 0; i < renderCommandExts.size(); ++i)
		{
			const RenderCommandExt& renderCommandExt = renderCommandExts.at(i);

			if (i == 0)
			{
				currentMeshBatch = &m_meshBatches.emplace_back();
				currentMeshBatch->batchType = MeshBatchType::VertexIndexBuffer | MeshBatchType::RenderPipeline;
				currentMeshBatch->first = 0;
				currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
				currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
				currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
				currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);

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
					currentMeshBatch->last = static_cast<uint32_t>(i);

					currentMeshBatch = &m_meshBatches.emplace_back();
					currentMeshBatch->first = static_cast<uint32_t>(i);
					currentMeshBatch->batchType = batchType;
				
					if (EnumValueContainsFlag(batchType, MeshBatchType::VertexIndexBuffer))
					{
						currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
						currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;

						VT_ENSURE(!currentMeshBatch->vertexBuffers.empty());
						VT_ENSURE(currentMeshBatch->indexBuffer);
					}
					
					if (EnumValueContainsFlag(batchType, MeshBatchType::RenderPipeline))
					{
						currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
						currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);

						VT_ENSURE(currentMeshBatch->renderPipeline);
						VT_ENSURE(currentMeshBatch->descriptorTable);
					}
				}
			}

			m_renderCommands.emplace_back(renderCommandExt);
		}

		// Set the last index for the last batch.
		if (currentMeshBatch)
		{
			currentMeshBatch->last = static_cast<uint32_t>(renderCommandExts.size());
		}
	}

	void MeshRenderer::Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters) const
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::CommandBuffer> commandBuffer = renderContext.GetRHICommandBuffer();

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

			for (uint32_t i = meshBatch.first; i < meshBatch.last; ++i)
			{
				const RenderCommand& renderCommand = m_renderCommands.at(i);

				// We use the firstInstance input to send the Primitive Index to the GPU.
				commandBuffer->DrawIndexed(renderCommand.indexCount, 1, renderCommand.firstIndex, renderCommand.vertexOffset, renderCommand.primitiveIndex);
			}
		}
	}
}
