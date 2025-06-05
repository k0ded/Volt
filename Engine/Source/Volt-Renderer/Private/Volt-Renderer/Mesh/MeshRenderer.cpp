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
		VT_PROFILE_FUNCTION();

		m_renderCommands.clear();
		m_meshBatches.clear();

		struct RenderCommandExt : public RenderCommand
		{
			size_t vertexIndexBufferHash;
			size_t subMeshHash;
			size_t renderPipelineHash;
		
			RefPtr<RHI::StorageBuffer> vertexBuffer;
			RefPtr<RHI::StorageBuffer> indexBuffer;
			RefPtr<RHI::RenderPipeline> renderPipeline;
		};

		Vector<RenderCommandExt> renderCommandExts;
		renderCommandExts.reserve(renderScene->GetRenderObjectCount());

		// First get all commands, their info and the requried hashes for sorting.
		for (const RenderPrimitiveData& renderPrimitive : *renderScene)
		{
			const SubMesh& subMesh = renderPrimitive.mesh->GetSubMeshes().at(renderPrimitive.subMeshIndex);

			auto& newCommand = renderCommandExts.emplace_back();
			newCommand.vertexBuffer = renderPrimitive.mesh->GetVertexPositionsBuffer()->GetResource();
			newCommand.indexBuffer = renderPrimitive.mesh->GetIndexBuffer()->GetResource();
			newCommand.indexCount = subMesh.indexCount;
			newCommand.firstIndex = subMesh.indexStartOffset;
			newCommand.vertexOffset = subMesh.vertexStartOffset;

			RHI::RenderPipelineCreateInfo renderPipelineInfo{};
			renderPipelineInfo.shaders = { ShaderMap::Get<MeshTestVS>(), ShaderMap::Get<MeshTestPS>() };
			newCommand.renderPipeline = PipelineStateCache::GetRenderPipeline(renderPipelineInfo);

			newCommand.vertexIndexBufferHash = Math::HashCombine(newCommand.vertexBuffer.GetHash(), newCommand.indexBuffer.GetHash());
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
				currentMeshBatch->vertexBuffer = renderCommandExt.vertexBuffer;
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
						currentMeshBatch->vertexBuffer = renderCommandExt.vertexBuffer;
						currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
					}
					
					if (EnumValueContainsFlag(batchType, MeshBatchType::RenderPipeline))
					{
						currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
						currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);
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

	void MeshRenderer::Render(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters)
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
				commandBuffer->BindVertexBuffers({ meshBatch.vertexBuffer }, 0);
				commandBuffer->BindIndexBuffer(meshBatch.indexBuffer);
			}

			for (uint32_t i = meshBatch.first; i < meshBatch.last; ++i)
			{
				const RenderCommand& renderCommand = m_renderCommands.at(i);

				commandBuffer->DrawIndexed(renderCommand.indexCount, 1, renderCommand.firstIndex, renderCommand.vertexOffset, 0);
			}
		}
	}
}
