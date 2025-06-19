#include "vrpch.h"

#include "Volt-Renderer/Mesh/MeshRenderer.h"
#include "Volt-Renderer/RenderScene.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/Renderer.h"
#include "Volt-Renderer/Texture/Texture2D.h"

#include <RenderCore/RenderGraph/RenderContext.h>
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
	}

	void MeshRenderer::BuildRenderCommands(Ref<RenderScene> renderScene, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommands(*renderScene, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommands(RenderScene& renderScene, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
	{
		constexpr auto func = [](const RenderPrimitiveData& primitive) { return true; };

		BuildRenderCommandsInternal(renderScene, func, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommandsWithFilter(Ref<RenderScene> renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommandsWithFilter(*renderScene, filterFunc, vertexShader, pixelShader, pipelineInfo);
	}

	void MeshRenderer::BuildRenderCommandsWithFilter(RenderScene& renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo)
	{
		BuildRenderCommandsInternal(renderScene, filterFunc, vertexShader, pixelShader, pipelineInfo);
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

	void MeshRenderer::BuildRenderCommandsInternal(RenderScene& renderScene, const PrimitveFilterFunc& filterFunc, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, const RHI::RenderPipelineCreateInfo& pipelineInfo /*= {}*/)
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
			Weak<RenderMaterial> renderMaterial;
		};

		Vector<RenderCommandExt> renderCommandExts;
		renderCommandExts.reserve(renderScene.GetRenderObjectCount());

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
			newCommand.indexCount = subMesh.indexCount;
			newCommand.firstIndex = subMesh.indexStartOffset;
			newCommand.vertexOffset = subMesh.vertexStartOffset;
			newCommand.primitiveIndex = renderScene.GetPrimitiveIndexFromID(renderPrimitive.id);

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

			// No pixel shader means that we will use the materials shader.
			if (!pixelShader)
			{
				pixelShader = renderPrimitive.material->GetPixelShader();
			}

			// If there still is no pixel shader, we will use the default one
			if (!pixelShader)
			{
				pixelShader = ShaderMap::Get<OpaqueDefaultPixelPS>();
			}

			renderPipelineInfo.shaders = { vertexShader, pixelShader };
			newCommand.renderPipeline = PipelineStateCache::GetRenderPipeline(renderPipelineInfo);
			newCommand.renderMaterial = renderPrimitive.material;

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
					currentMeshBatch->last = static_cast<uint32_t>(i);

					currentMeshBatch = &m_meshBatches.emplace_back();
					currentMeshBatch->first = static_cast<uint32_t>(i);
					currentMeshBatch->batchType = batchType;

					if (EnumValueContainsFlag(batchType, MeshBatchType::VertexIndexBuffer))
					{
						VT_ENSURE(!currentMeshBatch->vertexBuffers.empty());
						VT_ENSURE(currentMeshBatch->indexBuffer);

						currentMeshBatch->vertexBuffers = renderCommandExt.vertexBuffers;
						currentMeshBatch->indexBuffer = renderCommandExt.indexBuffer;
					}

					if (EnumValueContainsFlag(batchType, MeshBatchType::RenderPipeline))
					{
						VT_ENSURE(currentMeshBatch->renderPipeline);
						VT_ENSURE(currentMeshBatch->descriptorTable);

						currentMeshBatch->renderPipeline = renderCommandExt.renderPipeline;
						currentMeshBatch->descriptorTable = DescriptorTableCache::Get().GetOrCreateDescriptorTableForPipeline(renderCommandExt.renderPipeline);

						SetMaterialParametersInDescriptorTable(renderCommandExt.renderMaterial, renderCommandExt.renderPipeline, currentMeshBatch->descriptorTable);
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
}
