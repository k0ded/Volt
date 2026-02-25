#include "rcpch.h"
#include "RenderCore/RenderGraph/RenderGraphDebugger.h"
#include "RenderCore/RenderGraph/RenderGraph.h"

namespace Volt
{
	static ConsoleVariable<int32_t> s_enableRenderGraphDebugger(
		"r.RenderGraph.Debug", 
		1, 
		"Whether or not RenderGraph debugging is enabled or not.");

	void RenderGraphDebugger::Clear()
	{
		m_renderGraphPasses.clear();
		m_transientRenderGraphResources.clear();
		m_externalRenderGraphResources.clear();
	}

	void RenderGraphDebugger::ProcessRenderGraph(RenderGraph& renderGraph)
	{
		if (s_enableRenderGraphDebugger.GetValue() == 0)
		{
			return;
		}

		if (!VT_CHECK(renderGraph.IsCompiled()))
		{
			return;
		}

		Clear();

		const size_t numResources = renderGraph.m_resources.size();

		for (size_t i = 0; i < numResources; ++i)
		{
			RGResourceRef srcResource = renderGraph.m_resources[i];
			RenderGraphResource* resource = nullptr;

			if (srcResource->IsExternal() || srcResource->IsExtracted())
			{
				resource = &m_externalRenderGraphResources[srcResource->GetResourceID()];
			}
			else
			{
				resource = &m_transientRenderGraphResources[srcResource->GetResourceID()];
			}

			resource->resourceType = srcResource->GetResourceType();
			resource->isExternal = srcResource->m_isExternal;
			resource->isExtracted = srcResource->m_isExtracted;
			resource->isProduced = srcResource->m_isProduced;

			for (const RenderGraph::TempResourceLifetime& lifetime : renderGraph.resourceLifetimes)
			{
				if (srcResource == lifetime.resource)
				{
					resource->firstUsagePass = lifetime.firstPassIndex;
					resource->lastUsagePass = lifetime.lastPassIndex;
					break;
				}
			}

			if (resource->resourceType == RGResourceType::Buffer)
			{
				RGBufferRef buffer = ResourceCast<RGBuffer>(srcResource);
				resource->name = buffer->GetDesc().debugName;
			}
			else if (resource->resourceType == RGResourceType::Texture)
			{
				RGTextureRef texture = ResourceCast<RGTexture>(srcResource);
				resource->name = texture->GetDesc().debugName;
			}
			else if (resource->resourceType == RGResourceType::UniformBuffer)
			{
				RGUniformBufferRef uniformBuffer = ResourceCast<RGUniformBuffer>(srcResource);
				resource->name = uniformBuffer->GetDesc().debugName;
			}
		}

		const size_t numRenderPasses = renderGraph.m_renderPasses.size();
		m_renderGraphPasses.resize(numRenderPasses);

		for (size_t i = 0; i < numRenderPasses; ++i)
		{
			RGPassRef srcPass = renderGraph.m_renderPasses[i];

			RenderGraphPass& pass = m_renderGraphPasses[i];
			pass.passName = srcPass->m_name;
			pass.isCulled = srcPass->m_isCulled;
			pass.passFlags = srcPass->m_flags;

			srcPass->m_passParameters.EnumerateParameters([&pass](RenderGraphParameterDesc parameterDesc) mutable
			{
				switch (parameterDesc.GetType())
				{
					case ShaderParameterType::BufferSRV:
					{
						if (RGBufferSRVRef bufferSRV = parameterDesc.GetAs<RGBufferSRVRef>())
						{
							pass.resourceReads.emplace_back(bufferSRV->GetResource()->GetResourceID());
						}
						break;
					}

					case ShaderParameterType::BufferUAV:
					{
						if (RGBufferUAVRef bufferUAV = parameterDesc.GetAs<RGBufferUAVRef>())
						{
							pass.resourceWrites.emplace_back(bufferUAV->GetResource()->GetResourceID());
						}
						break;
					}

					case ShaderParameterType::TextureSRV:
					{
						if (RGTextureSRVRef textureSRV = parameterDesc.GetAs<RGTextureSRVRef>())
						{
							pass.resourceReads.emplace_back(textureSRV->GetResource()->GetResourceID());
						}
						break;
					}

					case ShaderParameterType::TextureUAV:
					{
						if (RGTextureUAVRef textureUAV = parameterDesc.GetAs<RGTextureUAVRef>())
						{
							pass.resourceWrites.emplace_back(textureUAV->GetResource()->GetResourceID());
						}
						break;
					}

					case ShaderParameterType::UniformBuffer:
					{
						if (RGUniformBufferRef uniformBuffer = parameterDesc.GetAs<RGUniformBufferRef>())
						{
							pass.resourceReads.emplace_back(uniformBuffer->GetResourceID());
						}
						break;
					}
				}
			});
		}
	}

	void RenderGraphDebugger::WaitForFinishedExecution() const
	{
	}
}
