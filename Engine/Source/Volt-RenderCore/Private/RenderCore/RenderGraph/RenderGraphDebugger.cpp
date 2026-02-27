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

			auto AddPassAccess = [this](uint32_t resourceId, uint32_t passIndex, bool isRead)
			{
				if (m_transientRenderGraphResources.contains(resourceId))
				{
					m_transientRenderGraphResources.at(resourceId).passAccesses.emplace_back(passIndex, isRead);
				}
				else if (m_externalRenderGraphResources.contains(resourceId))
				{
					m_externalRenderGraphResources.at(resourceId).passAccesses.emplace_back(passIndex, isRead);
				}
				else
				{
					VT_ENSURE_NO_ENTRY();
				}
			};

			auto GetIsReadFromAccessType = [](RGResourceAccess accessType) -> bool
			{
				switch (accessType)
				{
					case RGResourceAccess::None:
					case RGResourceAccess::IndirectArg:
					case RGResourceAccess::VertexBuffer:
					case RGResourceAccess::IndexBuffer:
					case RGResourceAccess::CopySrc:
						return true;

					case RGResourceAccess::CopyDst:
					case RGResourceAccess::Upload:
						return false;
				}

				VT_ENSURE_NO_ENTRY();
				return true;
			};

			srcPass->m_passParameters.EnumerateParameters([&pass, &AddPassAccess, &GetIsReadFromAccessType, passIndex = static_cast<int32_t>(i)](RenderGraphParameterDesc parameterDesc) mutable
			{
				switch (parameterDesc.GetType())
				{
					case ShaderParameterType::BufferSRV:
					{
						if (RGBufferSRVRef bufferSRV = parameterDesc.GetAs<RGBufferSRVRef>())
						{
							const uint32_t resourceId = bufferSRV->GetResource()->GetResourceID();
							AddPassAccess(resourceId, passIndex, true);
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::BufferUAV:
					{
						if (RGBufferUAVRef bufferUAV = parameterDesc.GetAs<RGBufferUAVRef>())
						{
							const uint32_t resourceId = bufferUAV->GetResource()->GetResourceID();
							AddPassAccess(resourceId, passIndex, false);
							pass.resourceWrites.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::TextureSRV:
					{
						if (RGTextureSRVRef textureSRV = parameterDesc.GetAs<RGTextureSRVRef>())
						{
							const uint32_t resourceId = textureSRV->GetResource()->GetResourceID();
							AddPassAccess(resourceId, passIndex, true);
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::TextureUAV:
					{
						if (RGTextureUAVRef textureUAV = parameterDesc.GetAs<RGTextureUAVRef>())
						{
							const uint32_t resourceId = textureUAV->GetResource()->GetResourceID();
							AddPassAccess(resourceId, passIndex, false);
							pass.resourceWrites.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::UniformBuffer:
					{
						if (RGUniformBufferRef uniformBuffer = parameterDesc.GetAs<RGUniformBufferRef>())
						{
							const uint32_t resourceId = uniformBuffer->GetResourceID();
							AddPassAccess(resourceId, passIndex, true);
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::BufferAccess:
					{
						if (RGBufferRef buffer = parameterDesc.GetAs<RGBufferRef>())
						{
							const uint32_t resourceId = buffer->GetResourceID();
							AddPassAccess(resourceId, passIndex, GetIsReadFromAccessType(parameterDesc.GetAccess()));
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::TextureAccess:
					{
						if (RGTextureRef texture = parameterDesc.GetAs<RGTextureRef>())
						{
							const uint32_t resourceId = texture->GetResourceID();
							AddPassAccess(resourceId, passIndex, GetIsReadFromAccessType(parameterDesc.GetAccess()));
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::UniformBufferAccess:
					{
						if (RGUniformBufferRef uniformBuffer = parameterDesc.GetAs<RGUniformBufferRef>())
						{
							const uint32_t resourceId = uniformBuffer->GetResourceID();
							AddPassAccess(resourceId, passIndex, GetIsReadFromAccessType(parameterDesc.GetAccess()));
							pass.resourceReads.emplace_back(resourceId);
						}
						break;
					}

					case ShaderParameterType::RenderTargets:
					{
						const ShaderParameterRenderTargetBindings& rtBindings = parameterDesc.GetAs<const ShaderParameterRenderTargetBindings&>();
		
						for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
						{
							RGTextureRef renderTarget = rtBindings.renderTargets[i].texture;
							if (renderTarget != nullptr)
							{
								const uint32_t resourceId = renderTarget->GetResourceID();
								AddPassAccess(resourceId, passIndex, false);
								pass.renderTargets.emplace_back(resourceId);
							}
						}

						RGTextureRef depthTarget = rtBindings.depthTarget.texture;
						if (depthTarget != nullptr)
						{
							const uint32_t resourceId = depthTarget->GetResourceID();
							AddPassAccess(resourceId, passIndex, false);
							pass.renderTargets.emplace_back(resourceId);
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
