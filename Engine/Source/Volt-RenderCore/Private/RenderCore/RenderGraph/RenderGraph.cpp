#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/RenderGraph/GPUReadbackBuffer.h"
#include "RenderCore/RenderGraph/GPUReadbackTexture.h"

#include "RenderCore/TransientResourceSystem/TransientResource.h"

#include "RenderCore/CommandBufferPool.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/DeviceQueue.h>
#include <RHIModule/RHIFeatures.h>

#include <JobSystem/TaskGraph.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/ComparisonHelpers.h>
#include <CoreUtilities/Malloc.h>
#include <CoreUtilities/MemoryUtility.h>

/*
	These are the synchronization cases referenced and handeled in RenderGraph::Compile.

	### Case 1:

	- If it’s an image resource AND the previous AND current usage are READ operations, no barrier is required.

	### Case 2:

	- If it’s an image resource AND the previous AND current usage are WRITE operations of the same type, a global barrier should be inserted.

	### Case 3:

	- If it’s a buffer resource AND the previous AND current usage are READ operations, no barrier is required.

	### Case 4:

	- If it’s a buffer resource AND the previous AND current usage are WRITE operations, a global barrier should be inserted.

	### Case 5:

	- If it’s a buffer resource AND the previous usage was a READ operation AND the current usage is a WRITE operation, a global barrier should be inserted.

	### Case 6:

	- If it’s a buffer resource AND the previous usage was a WRITE operation AND the current usage is a READ operation, a global barrier should be inserted.

	### Case 7: Resource A is created in render pass B

	If a resource is created in a render pass, we assume that the resource will be written to in the pass.

	- If it’s a depth resource AND it’s a rasterization pass -> transition to a DEPTH_WRITE state
	- If it’s a depth resource AND it’s a compute pass -> transition to a SHADER_WRITE state
	- If it’s a color resource AND it’s a rasterization pass -> transition to a COLOR_WRITE state
	- If it’s a color resource AND it’s a compute pass -> transition to a SHADER_WRITE state
	- If it’s a buffer resource -> transition to a SHADER_WRITE state

	### Case 8: Resource A is read in render pass B

	If a resource is marked as read in a render pass, the resource will be transitioned into a read state.

	- All resources will be transitioned into a SHADER_READ state

	### Case 9: Resource A is written to, but not created in render pass B

	If a resource is marked as write in a render pass, but not created in that render pass, the resource will be transitioned into a write state.

	- If it’s a depth resource AND it’s a rasterization pass -> transition to a DEPTH_WRITE state
	- If it’s a depth resource AND it’s a compute pass -> transition to a SHADER_WRITE state
	- If it’s a color resource AND it’s a rasterization pass -> transition to a COLOR_WRITE state
	- If it’s a color resource AND it’s a compute pass -> transition to a SHADER_WRITE state
	- If it’s a buffer resource -> transition to a SHADER_WRITE state
	- If it’s a compute pass AND the previous pass was a compute write pass -> insert a memory barrier with the correct state.
	- If it’s a compute pass AND the previous pass was a compute read pass -> insert a memory barrier with the correct state.
*/

namespace Volt
{
	static ConsoleVariable<int32_t> g_renderGraphForceSingleThreadedExecution(
		"r.RenderGraph.ForceSingleThreadedExecution",
		0,
		"Whether or not to force single threaded execution of the RenderGraph.");

	static ConsoleVariable<int32_t> g_renderGraphForceFullBarriersBetweenPasses(
		"r.RenderGraph.ForceFullBarriersBetweenPasses",
		0,
		""
	);

	inline RHI::ResourceState GetWriteStateForRasterizedTexture(RGResourceRef resource)
	{
		VT_ENSURE(resource->GetResourceType() == RGResourceType::Texture);

		RGTextureRef renderGraphTexture = reinterpret_cast<RGTextureRef>(resource);

		RHI::ResourceState resultState;

		// If depth is one, then it's a 2D texture, meaning it's a depth stencil or render target.
		if (renderGraphTexture->GetDesc().depth == 1)
		{
			if (RHI::Utility::IsDepthFormat(renderGraphTexture->GetDesc().format))
			{
				resultState.access = RHI::BarrierAccess::DepthStencilWrite | RHI::BarrierAccess::DepthStencilRead;
				resultState.stage = RHI::BarrierStage::DepthStencil;
				resultState.layout = RHI::ImageLayout::DepthStencilWrite;
			}
			else
			{
				resultState.access = RHI::BarrierAccess::RenderTarget;
				resultState.stage = RHI::BarrierStage::RenderTarget;
				resultState.layout = RHI::ImageLayout::RenderTarget;
			}
		}
		else
		{
			resultState.access = RHI::BarrierAccess::ShaderWrite;
			resultState.stage = RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader;
			resultState.layout = RHI::ImageLayout::ShaderWrite;

			if (RHI::RHICanUseMeshShaders())
			{
				resultState.stage |= RHI::BarrierStage::MeshShader | RHI::BarrierStage::AmplificationShader;
			}
		}

		return resultState;
	}

	inline void SetupResourceStateFromAccess(RGResourceAccess accessType, RHI::ResourceState& outState)
	{
		if (accessType == RGResourceAccess::IndirectArg)
		{
			outState.access = RHI::BarrierAccess::IndirectArgument;
			outState.stage = RHI::BarrierStage::DrawIndirect;
		}
		else if (accessType == RGResourceAccess::IndexBuffer)
		{
			outState.access = RHI::BarrierAccess::IndexBuffer;
			outState.stage = RHI::BarrierStage::IndexInput;
		}
		else if (accessType == RGResourceAccess::VertexBuffer)
		{
			outState.access = RHI::BarrierAccess::VertexBuffer;
			outState.stage = RHI::BarrierStage::VertexInput;
		}
		else if (accessType == RGResourceAccess::CopyDst)
		{
			outState.access = RHI::BarrierAccess::CopyDest;
			outState.stage = RHI::BarrierStage::Copy;
			outState.layout = RHI::ImageLayout::CopyDest;
		}
		else if (accessType == RGResourceAccess::CopySrc)
		{
			outState.access = RHI::BarrierAccess::CopySource;
			outState.stage = RHI::BarrierStage::Copy;
			outState.layout = RHI::ImageLayout::CopySource;
		}
	}

	inline RHI::BarrierStage GetBarrierStageFromPassFlags(RenderGraphPassFlags flags)
	{
		RHI::BarrierStage resultStage = RHI::BarrierStage::None;

		if (EnumValueContainsFlag(RenderGraphPassFlags::Compute, flags))
		{
			resultStage = RHI::BarrierStage::ComputeShader;
		}
		else
		{
			resultStage = RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader;

			if (RHI::RHICanUseMeshShaders())
			{
				resultStage |= RHI::BarrierStage::MeshShader | RHI::BarrierStage::AmplificationShader;
			}
		}

		return resultStage;
	}

	inline void InitializeImageBarrierSubresourceFromSRV(RGResourceSRVRef textureSRV, RHI::ImageSubResource& subResource)
	{
		RGTextureSRVRef textureSRVRef = reinterpret_cast<RGTextureSRVRef>(textureSRV);
		const RGTextureSRVDesc& srvDesc = textureSRVRef->GetDesc();

		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureSRV->GetResource());
		const RGTextureDesc& textureDesc = texture->GetDesc();

		subResource.baseMipLevel = srvDesc.baseMipLevel;
		subResource.levelCount = srvDesc.mipCount == RHI::ImageViewDesc::MipCountMax ? textureDesc.mips - subResource.baseMipLevel : srvDesc.mipCount;
		subResource.layerCount = srvDesc.layerCount == RHI::ImageViewDesc::LayerCountMax ? textureDesc.layers - subResource.baseArrayLayer : srvDesc.layerCount;
		subResource.baseArrayLayer = srvDesc.baseArrayLayer;
		subResource.baseMipLevel = srvDesc.baseMipLevel;
	}

	inline void InitializeImageBarrierSubresourceFromUAV(RGResourceUAVRef textureUAV, RHI::ImageSubResource& subResource)
	{
		RGTextureUAVRef textureUAVRef = reinterpret_cast<RGTextureUAVRef>(textureUAV);
		const RGTextureUAVDesc& srvDesc = textureUAVRef->GetDesc();

		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureUAV->GetResource());
		const RGTextureDesc& textureDesc = texture->GetDesc();

		subResource.baseMipLevel = srvDesc.baseMipLevel;
		subResource.levelCount = srvDesc.mipCount == RHI::ImageViewDesc::MipCountMax ? textureDesc.mips - subResource.baseMipLevel : srvDesc.mipCount;
		subResource.layerCount = srvDesc.layerCount == RHI::ImageViewDesc::LayerCountMax ? textureDesc.layers - subResource.baseArrayLayer : srvDesc.layerCount;
		subResource.baseArrayLayer = srvDesc.baseArrayLayer;
		subResource.baseMipLevel = srvDesc.baseMipLevel;
	}

	RenderGraph::RenderGraph()
	{
		VT_PROFILE_FUNCTION();

		m_executionFence = RHI::Fence::Create();
	}

	RenderGraph::~RenderGraph()
	{}

	RenderGraph::RenderGraph(RenderGraph&& other) noexcept
		: m_registeredExternalResources(std::move(other.m_registeredExternalResources)),
		m_resourceAllocator(std::move(other.m_resourceAllocator)),
		m_resourceAccessorAllocator(std::move(other.m_resourceAccessorAllocator)),
		m_passParametersAllocator(std::move(other.m_passParametersAllocator)),
		m_passAllocator(std::move(other.m_passAllocator)),
		m_renderPasses(std::move(other.m_renderPasses)),
		m_resources(std::move(other.m_resources)),
		m_compiledRenderPasses(std::move(other.m_compiledRenderPasses)),
		m_executionFence(std::move(other.m_executionFence)),
		m_textureExtractions(std::move(other.m_textureExtractions)),
		m_bufferExtractions(std::move(other.m_bufferExtractions)),
		m_standaloneBarriers(std::move(other.m_standaloneBarriers)),
		m_standaloneMarkers(std::move(other.m_standaloneMarkers)),
		m_temporaryDataAllocator(std::move(other.m_temporaryDataAllocator)),
		m_resourceSRVs(std::move(other.m_resourceSRVs)),
		m_resourceUAVs(std::move(other.m_resourceUAVs)),
		m_resourceManager(std::move(other.m_resourceManager))
	{}

	RenderGraph& RenderGraph::operator=(RenderGraph&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		m_registeredExternalResources = std::move(other.m_registeredExternalResources);
		m_resourceAllocator = std::move(other.m_resourceAllocator);
		m_resourceAccessorAllocator = std::move(other.m_resourceAccessorAllocator);
		m_passParametersAllocator = std::move(other.m_passParametersAllocator);
		m_passAllocator = std::move(other.m_passAllocator);
		m_renderPasses = std::move(other.m_renderPasses);
		m_resources = std::move(other.m_resources);
		m_compiledRenderPasses = std::move(other.m_compiledRenderPasses);
		m_executionFence = std::move(other.m_executionFence);
		m_textureExtractions = std::move(other.m_textureExtractions);
		m_bufferExtractions = std::move(other.m_bufferExtractions);
		m_standaloneBarriers = std::move(other.m_standaloneBarriers);
		m_standaloneMarkers = std::move(other.m_standaloneMarkers);
		m_temporaryDataAllocator = std::move(other.m_temporaryDataAllocator);
		m_resourceSRVs = std::move(other.m_resourceSRVs);
		m_resourceUAVs = std::move(other.m_resourceUAVs);
		m_resourceManager = std::move(other.m_resourceManager);

		return *this;
	}

	RGBuffer* RenderGraph::CreateBuffer(const RGBufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(desc.count * desc.elementSize > 0);

		RGBufferRef buffer = m_resourceAllocator.Allocate<RGBuffer>(desc);
		m_resources.emplace_back(buffer);

		return buffer;
	}

	RGTexture* RenderGraph::CreateTexture(const RGTextureDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(RHI::Utility::IsDepthFormat(desc.format) ? (desc.usage != RHI::ImageUsage::AttachmentStorage && desc.usage != RHI::ImageUsage::Storage) : true, 
			"A texture with a depth format may not be used for UAV access!");

		RGTextureRef texture = m_resourceAllocator.Allocate<RGTexture>(desc);
		m_resources.emplace_back(texture);

		return texture;
	}

	RGUniformBufferRef RenderGraph::CreateUniformBuffer(const RGUniformBufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		RGUniformBufferRef uniformBuffer = m_resourceAllocator.Allocate<RGUniformBuffer>(desc);
		m_resources.emplace_back(uniformBuffer);

		return uniformBuffer;
	}

	void RenderGraph::TransitionExternalResources()
	{
		VT_PROFILE_FUNCTION();

		auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

		for (const RGResourceRef resource : m_resources)
		{
			if (!resource->m_isExternal)
			{
				continue;
			}

			if (resource->GetResourceType() == RGResourceType::Texture)
			{
				RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
				TransitionExternalResource(textureResource);
			}
			else if (resource->GetResourceType() == RGResourceType::Buffer)
			{
				RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
				TransitionExternalResource(bufferResource);
			}
			else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
			{
				RGUniformBufferRef bufferResource = reinterpret_cast<RGUniformBufferRef>(resource);
				TransitionExternalResource(bufferResource);
			}
		}
	}

	void RenderGraph::PrepareResourcesForExecution()
	{
		VT_PROFILE_FUNCTION();

		// At this point we know all resources that will be referenced, and we can create them accordingly.
		for (auto resource : m_resources)
		{
			const bool shouldResourceBeCreated =
				!resource->m_isExternal &&
				(resource->m_refCount > 0 || resource->m_isExtracted);

			if (shouldResourceBeCreated)
			{
				if (resource->GetResourceType() == RGResourceType::Texture)
				{
					RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
					m_resourceManager.AllocateResource(textureResource);
				}
				else if (resource->GetResourceType() == RGResourceType::Buffer)
				{
					RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
					m_resourceManager.AllocateResource(bufferResource);
				}
				else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
				{
					m_resourceManager.AllocateResource(reinterpret_cast<RGUniformBufferRef>(resource));
				}
			}
		}
	}

	template<typename T>
	inline RHI::ImageViewDesc GetImageViewDesc(const RGTextureDesc& textureDesc, const T& desc)
 	{
		RHI::ImageViewDesc viewDesc{};
		viewDesc.baseMipLevel = desc.baseMipLevel;
		viewDesc.baseArrayLayer = desc.baseArrayLayer;
		viewDesc.mipCount = desc.mipCount;
		viewDesc.layerCount = desc.layerCount;

		if (textureDesc.imageType == RHI::ResourceType::Image1D)
		{
			if (textureDesc.layers == 1 || viewDesc.layerCount == 1)
			{
				viewDesc.viewType = RHI::ImageViewType::View1D;
			}
			else
			{
				viewDesc.viewType = RHI::ImageViewType::View1DArray;
			}
		}
		else if (textureDesc.imageType == RHI::ResourceType::Image2D)
		{
			if (textureDesc.layers == 1 || viewDesc.layerCount == 1)
			{
				viewDesc.viewType = RHI::ImageViewType::View2D;
			}
			else
			{
				if (textureDesc.isCubeMap)
				{
					viewDesc.viewType = RHI::ImageViewType::ViewCube;
				}
				else
				{
					viewDesc.viewType = RHI::ImageViewType::View2DArray;
				}
			}
		}
		else if (textureDesc.imageType == RHI::ResourceType::Image3D)
		{
			if (textureDesc.layers == 1 || viewDesc.layerCount == 1)
			{
				viewDesc.viewType = RHI::ImageViewType::View3D;
			}
			else
			{
				viewDesc.viewType = RHI::ImageViewType::View3DArray;
			}
		}

		return viewDesc;
	}

	void RenderGraph::CreateResourceViews()
	{
		VT_PROFILE_FUNCTION();

		for (RGResourceSRVRef resourceSRV : m_resourceSRVs)
		{
			RGResourceRef resource = resourceSRV->GetResource();
			if (resource->GetResourceType() == RGResourceType::Buffer)
			{
				RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
				RGBufferSRVRef bufferSRV = reinterpret_cast<RGBufferSRVRef>(resourceSRV);

				// Make sure the buffer resource has been assigned.
				if (bufferResource->GetRHIResource() != nullptr)
				{
					RHI::BufferViewDesc viewDesc{};
					viewDesc.bufferFormat = bufferSRV->GetDesc().format;
					viewDesc.size = bufferSRV->GetDesc().size;
					viewDesc.offset = bufferSRV->GetDesc().offset;
					bufferSRV->AssignRHIView(bufferResource->GetRHIResource()->GetOrCreateView(viewDesc));
				}
			}
			else if (resource->GetResourceType() == RGResourceType::Texture)
			{
				RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
				RGTextureSRVRef textureSRV = reinterpret_cast<RGTextureSRVRef>(resourceSRV);

				// Make sure the buffer resource has been assigned.
				if (textureResource->GetRHIResource() != nullptr)
				{
					const RGTextureDesc& textureDesc = textureResource->GetDesc();
					const RGTextureSRVDesc& srvDesc = textureSRV->GetDesc();

					const RHI::ImageViewDesc viewDesc = GetImageViewDesc(textureDesc, srvDesc);
					textureSRV->AssignRHIView(textureResource->GetRHIResource()->GetOrCreateView(viewDesc));
				}
			}
			else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
			{
				RGUniformBufferRef uniformBufferResource = reinterpret_cast<RGUniformBufferRef>(resource);
				RGUniformBufferSRVRef uniformBufferSRV = reinterpret_cast<RGUniformBufferSRVRef>(resourceSRV);

				if (uniformBufferResource->GetRHIResource())
				{
					RHI::BufferViewDesc desc{};
					desc.offset = uniformBufferSRV->GetDesc().offset;
					uniformBufferSRV->AssignRHIView(uniformBufferResource->GetRHIResource()->GetOrCreateView(desc));
				}
			}
			else
			{
				VT_ENSURE(false);
			}
		}

		for (RGResourceUAVRef resourceUAV : m_resourceUAVs)
		{
			RGResourceRef resource = resourceUAV->GetResource();
			if (resource->GetResourceType() == RGResourceType::Buffer)
			{
				RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
				RGBufferUAVRef bufferUAV = reinterpret_cast<RGBufferUAVRef>(resourceUAV);

				// Make sure the buffer resource has been assigned.
				if (bufferResource->GetRHIResource())
				{
					RHI::BufferViewDesc viewDesc{};
					viewDesc.bufferFormat = bufferUAV->GetDesc().format;
					viewDesc.size = bufferUAV->GetDesc().size;
					viewDesc.offset = bufferUAV->GetDesc().offset;
					bufferUAV->AssignRHIView(bufferResource->GetRHIResource()->GetOrCreateView(viewDesc));
				}
			}
			else if (resource->GetResourceType() == RGResourceType::Texture)
			{
				RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
				RGTextureUAVRef textureUAV = reinterpret_cast<RGTextureUAVRef>(resourceUAV);

				// Make sure the buffer resource has been assigned.
				if (textureResource->GetRHIResource() != nullptr)
				{
					const RGTextureDesc& textureDesc = textureResource->GetDesc();
					const RGTextureUAVDesc& srvDesc = textureUAV->GetDesc();

					const RHI::ImageViewDesc viewDesc = GetImageViewDesc(textureDesc, srvDesc);
					textureUAV->AssignRHIView(textureResource->GetRHIResource()->GetOrCreateView(viewDesc));
				}
			}
			else
			{
				VT_ENSURE(false);
			}
		}

		// Make sure a view is created for each render target.
		// #TODO_Ivar: Need some way of knowing which textures are render targets.
#if 0
		for (RGPassRef pass : m_renderPasses)
		{
			for (RGTextureRef renderTarget : pass->GetResourceRenderTargetAccesses())
			{
				if (renderTarget->GetRHIResource() != nullptr)
				{
					RHI::ImageViewDesc viewDesc{};
					renderTarget->GetRHIResource()->GetOrCreateView(viewDesc);
				}
			}
		}
#endif
	}

	void RenderGraph::SetupPass(RGPassRef pass)
	{
		VT_PROFILE_FUNCTION();

		SetupPassParameters(pass);
		SetupPassDependencies(pass);
	}

	void RenderGraph::SetupPassParameters(RGPassRef pass)
	{
		VT_PROFILE_FUNCTION();

		pass->m_passParameters.EnumerateParameters([pass, this](RenderGraphParameterDesc parameterDesc) mutable
		{
			const RHI::BarrierStage passStage = GetBarrierStageFromPassFlags(pass->m_flags);

			switch (parameterDesc.GetType())
			{
				case ShaderParameterType::BufferSRV:
				{
					if (RGBufferSRVRef bufferSRV = parameterDesc.GetAs<RGBufferSRVRef>())
					{
						RGBufferState& bufferState = pass->GetOrCreateBufferState(bufferSRV);
						bufferState.subResourceState.AddState(passStage, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);
					}

					break;
				}

				case ShaderParameterType::BufferUAV:
				{
					if (RGBufferUAVRef bufferUAV = parameterDesc.GetAs<RGBufferUAVRef>())
					{
						RGBufferState& bufferState = pass->GetOrCreateBufferState(bufferUAV);
						bufferState.subResourceState.AddState(passStage, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);
					}

					break;
				}

				case ShaderParameterType::TextureSRV:
				{
					if (RGTextureSRVRef textureSRV = parameterDesc.GetAs<RGTextureSRVRef>())
					{
						RGTextureState& textureState = pass->GetOrCreateTextureState(textureSRV);

						textureState.EnumerateSubResourceRange(textureSRV->GetSubResourceRange(), [&](RGSubResourceState*& subResourceState) 
						{
							if (!subResourceState)
							{
								subResourceState = AllocateSubResourceState();
							}

							subResourceState->AddState(passStage, RHI::BarrierAccess::ShaderRead, RHI::ImageLayout::ShaderRead);
						});
					}
	
					break;
				}

				case ShaderParameterType::TextureUAV:
				{
					if (RGTextureUAVRef textureUAV = parameterDesc.GetAs<RGTextureUAVRef>())
					{
						RGTextureState& textureState = pass->GetOrCreateTextureState(textureUAV);

						textureState.EnumerateSubResourceRange(textureUAV->GetSubResourceRange(), [&](RGSubResourceState*& subResourceState)
						{
							if (!subResourceState)
							{
								subResourceState = AllocateSubResourceState();
							}

							subResourceState->AddState(passStage, RHI::BarrierAccess::ShaderWrite, RHI::ImageLayout::ShaderWrite);
						});
					}

					break;
				}

				case ShaderParameterType::UniformBuffer:
				{
					if (RGUniformBufferRef uniformBuffer = parameterDesc.GetAs<RGUniformBufferRef>())
					{
						RGBufferState& bufferState = pass->GetOrCreateBufferState(uniformBuffer);
						bufferState.subResourceState.AddState(passStage, RHI::BarrierAccess::UniformBuffer, RHI::ImageLayout::Undefined);
					}

					break;
				}

				case ShaderParameterType::BufferAccess:
				{
					if (RGBufferRef buffer = parameterDesc.GetAs<RGBufferRef>())
					{
						RGBufferState& bufferState = pass->GetOrCreateBufferState(buffer, RGResourceAccessType::Read);

						RHI::ResourceState newState{};
						SetupResourceStateFromAccess(parameterDesc.GetAccess(), newState);

						bufferState.subResourceState.AddState(newState.stage, newState.access, newState.layout);
					}

					break;
				}

				case ShaderParameterType::TextureAccess:
				{
					if (RGTextureRef texture = parameterDesc.GetAs<RGTextureRef>())
					{
						const RGTextureDesc& textureDesc = texture->GetDesc();
						const RGTextureSubResourceRange subResourceRange
						{
							.baseMipLevel = 0,
							.baseArrayLayer = 0,
							.mipCount = textureDesc.mips,
							.layerCount = textureDesc.layers
						};

						RHI::ResourceState newState{};
						SetupResourceStateFromAccess(parameterDesc.GetAccess(), newState);

						RGTextureState& textureState = pass->GetOrCreateTextureState(texture, RGResourceAccessType::Write);
						textureState.EnumerateSubResourceRange(subResourceRange, [&](RGSubResourceState*& subResourceState) 
						{
							if (!subResourceState)
							{
								subResourceState = AllocateSubResourceState();
							}

							subResourceState->AddState(newState.stage, newState.access, newState.layout);
						});
					}

					break;
				}

				case ShaderParameterType::UniformBufferAccess:
				{
					if (RGUniformBufferRef uniformBuffer = parameterDesc.GetAs<RGUniformBufferRef>())
					{
						RGBufferState& bufferState = pass->GetOrCreateBufferState(uniformBuffer);
						
						RHI::ResourceState newState{};
						SetupResourceStateFromAccess(parameterDesc.GetAccess(), newState);

						bufferState.subResourceState.AddState(newState.stage, newState.access, newState.layout);
					}

					break;
				}

				case ShaderParameterType::RenderTargets:
				{
					VT_ENSURE(!EnumValueContainsFlag(pass->m_flags, RenderGraphPassFlags::Compute));

					const ShaderParameterRenderTargetBindings& rtBindings = parameterDesc.GetAs<const ShaderParameterRenderTargetBindings&>();

					for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
					{
						RGTextureRef renderTarget = rtBindings.renderTargets[i].texture;

						if (renderTarget != nullptr)
						{
							VT_ENSURE_MSG(renderTarget->GetDesc().usage == RHI::ImageUsage::Attachment
								|| renderTarget->GetDesc().usage == RHI::ImageUsage::AttachmentStorage,
								"Render Targets must have a Attachment usage type!");

							const RHI::ResourceState newState = GetWriteStateForRasterizedTexture(renderTarget);

							RGTextureState& textureState = pass->GetOrCreateTextureState(renderTarget, RGResourceAccessType::Write);
							textureState.EnumerateSubResourceRange(rtBindings.renderTargets[i].subResourceRange, [&](RGSubResourceState*& subResourceState) 
							{
								if (!subResourceState)
								{
									subResourceState = AllocateSubResourceState();
								}

								subResourceState->AddState(RHI::BarrierStage::RenderTarget, newState.access, newState.layout);
							});
						}
					}

					RGTextureRef depthTarget = rtBindings.depthTarget.texture;

					if (depthTarget != nullptr)
					{
						VT_ENSURE_MSG(depthTarget->GetDesc().usage == RHI::ImageUsage::Attachment
							|| depthTarget->GetDesc().usage == RHI::ImageUsage::AttachmentStorage,
							"Render Targets must have a Attachment usage type!");

						const RHI::ResourceState newState = GetWriteStateForRasterizedTexture(depthTarget);

						RGTextureState& textureState = pass->GetOrCreateTextureState(depthTarget, RGResourceAccessType::Write);
						textureState.EnumerateSubResourceRange(rtBindings.depthTarget.subResourceRange, [&](RGSubResourceState*& subResourceState) 
						{
							if (!subResourceState)
							{
								subResourceState = AllocateSubResourceState();
							}

							subResourceState->AddState(RHI::BarrierStage::DepthStencil, newState.access, newState.layout);
						});
					}

					break;
				}
			}
		});
	}

	void RenderGraph::SetupPassDependencies(RGPassRef pass)
	{
		VT_PROFILE_FUNCTION();
		/*
			Walk through resource states and add their last accesses as dependencies to this pass.
		*/

		for (uint32_t stateIndex = 0; RGBufferState& bufferState : pass->m_bufferStates)
		{
			// Add pass dependency and set the new last access state.
			{
				RGResourceAccessState* lastAccess = nullptr;

				if (bufferState.bufferType == RGResourceType::Buffer)
				{
					lastAccess = &bufferState.buffer->lastAccess;

					if (bufferState.buffer->firstAccess == nullptr)
					{
						bufferState.buffer->firstAccess = &bufferState.subResourceState;
					}
				}
				else
				{
					lastAccess = &bufferState.uniformBuffer->lastAccess;

					if (bufferState.uniformBuffer->firstAccess == nullptr)
					{
						bufferState.uniformBuffer->firstAccess = &bufferState.subResourceState;
					}
				}

				VT_ENSURE(lastAccess != nullptr);

				AddPassDependency(pass, bufferState.bufferType, 0, bufferState.subResourceState, *lastAccess);

				RGResourceAccessState accessState{};
				accessState.pass = pass;
				accessState.stateIndex = stateIndex++;
				accessState.accessType = bufferState.accessType;

				*lastAccess = accessState;
			}

			// Add references to the resource
			if (bufferState.bufferType == RGResourceType::Buffer)
			{
				bufferState.buffer->m_refCount += bufferState.refCount;
			}
			else
			{
				bufferState.uniformBuffer->m_refCount += bufferState.refCount;
			}
		}

		for (uint32_t stateIndex = 0; RGTextureState& textureState : pass->m_textureStates)
		{
			// Add pass dependency and set the new last access state.
			{
				RGTextureResourceAccessState& lastAccess = textureState.texture->lastAccess;

				textureState.EnumerateSubResources([&](RGSubResourceState& subResource, uint32_t subResourceIndex)
				{
					if (textureState.texture->firstAccess[subResourceIndex] == nullptr)
					{
						textureState.texture->firstAccess[subResourceIndex] = &subResource;
					}

					AddPassDependency(pass, RGResourceType::Texture, subResourceIndex, subResource, lastAccess[subResourceIndex]);

					RGResourceAccessState accessState{};
					accessState.pass = pass;
					accessState.stateIndex = stateIndex;
					accessState.accessType = textureState.accessType;

					lastAccess[subResourceIndex] = accessState;
				});

				stateIndex++;
			}

			// Add references to the resource
			textureState.texture->m_refCount += textureState.refCount;
		}
	}

	void RenderGraph::CullPasses()
	{
		VT_PROFILE_FUNCTION();

		Vector<RGPassRef> unreferencedPasses{};
		for (RGPassRef pass : m_renderPasses)
		{
			if (pass->m_refCount == 0)
			{
				unreferencedPasses.emplace_back(pass);
			}
		}

		auto passShouldBeCulled = [](RGPassRef pass) -> bool
		{
			if (EnumValueContainsFlag(pass->m_flags, RenderGraphPassFlags::NeverCull))
			{
				return false;
			}

			for (const RGBufferState& bufferState : pass->m_bufferStates)
			{
				bool resourceExtractedOrExternal = false;

				if (bufferState.bufferType == RGResourceType::Buffer)
				{
					resourceExtractedOrExternal = bufferState.buffer->m_isExtracted || bufferState.buffer->m_isExternal;
				}
				else
				{
					resourceExtractedOrExternal = bufferState.uniformBuffer->m_isExtracted || bufferState.uniformBuffer->m_isExternal;
				}

				if (resourceExtractedOrExternal)
				{
					return false;
				}
			}
			
			for (const RGTextureState& textureState : pass->m_textureStates)
			{
				if (textureState.texture->m_isExternal || textureState.texture->m_isExtracted)
				{
					return false;
				}
			}

			return true;
		};

		while (!unreferencedPasses.empty())
		{
			RGPassRef unreferencedPass = unreferencedPasses.back();
			unreferencedPasses.pop_back();

			// Check if any resources / flags leads to pass not being culled.
			if (!passShouldBeCulled(unreferencedPass))
			{
				continue;
			}

			// Remove the resource references that this pass has
			for (const RGBufferState& bufferState : unreferencedPass->m_bufferStates)
			{
				if (bufferState.bufferType == RGResourceType::Buffer)
				{
					bufferState.buffer->m_refCount -= bufferState.refCount;
				}
				else
				{
					bufferState.uniformBuffer->m_refCount -= bufferState.refCount;
				}
			}

			for (const RGTextureState& textureState : unreferencedPass->m_textureStates)
			{
				textureState.texture->m_refCount -= textureState.refCount;
			}

			// Remove references from pass dependencies.
			for (RGPassRef passDep : unreferencedPass->m_passDependencies)
			{
				passDep->m_refCount--;

				// Add unreferenced passes to stack.
				if (passDep->m_refCount == 0)
				{
					passDep->m_isCulled = true;
					unreferencedPasses.emplace_back(passDep);
				}
			}
		}
	}

	void RenderGraph::BuildPassBarriers()
	{
		VT_PROFILE_FUNCTION();

		constexpr auto isBarrierRequired = [](const RGSubResourceState& subResourceState)
		{
			const bool bothAreUniformBuffer = IsEqualToAll(subResourceState.previousState.access, RHI::BarrierAccess::UniformBuffer) &&
				IsEqualToAll(subResourceState.state.access, RHI::BarrierAccess::UniformBuffer);

			const bool bothAreShaderRead = IsEqualToAll(subResourceState.previousState.access == RHI::BarrierAccess::ShaderRead) &&
				IsEqualToAll(subResourceState.state.access, RHI::BarrierAccess::ShaderRead);

			// If previous state was read, new state is read and the layout is the same, no barrier is required.
			if ((bothAreUniformBuffer || bothAreShaderRead) &&
				subResourceState.previousState.layout == subResourceState.state.layout)
			{
				return false;
			}

			return true;
		};

		constexpr auto isLayoutTransitionRequired = [](const RGSubResourceState& subResourceState)
		{
			return subResourceState.previousState.layout != subResourceState.state.layout;
		};

		constexpr auto canMergeSubResourceBarriers = [](RHI::ResourceBarrierInfo* activeBarrier, const RGSubResourceState& newState, uint32_t subResourceIndex, uint32_t prevSubResourceIndex) -> bool
		{
			// No previous barrier.
			if (activeBarrier == nullptr)
			{
				return false;
			}

			const bool barrierIsGlobal = activeBarrier->type == RHI::BarrierType::Global;

			// If a layout transition is required and the active barrier doesn't support it,
			// we need another barrier
			if (isLayoutTransitionRequired(newState))
			{
				if (barrierIsGlobal)
				{
					return false;
				}

				// Only merge if src and dst layout stages match.
				if (activeBarrier->imageBarrier().dstLayout != newState.state.layout ||
					activeBarrier->imageBarrier().srcLayout != newState.previousState.layout)
				{
					return false;
				}
			}

			// Sub resource range must be continuous if the barrier isn't a global barrier.
			if (!barrierIsGlobal && prevSubResourceIndex + 1 != subResourceIndex)
			{
				return false;
			}

			return true;
		};

		m_compiledRenderPasses.resize(m_renderPasses.size());
	
		for (size_t passIndex = 0; passIndex < m_renderPasses.size(); ++passIndex)
		{
			RGPassRef pass = m_renderPasses[passIndex];
			RGCompiledPass& compiledPass = m_compiledRenderPasses[passIndex];

			compiledPass.SetName(pass->m_name);

			// Skip culled passes.
			if (pass->m_isCulled)
			{
				continue;
			}

			for (const RGBufferState& bufferState : pass->m_bufferStates)
			{
				if (!isBarrierRequired(bufferState.subResourceState))
				{
					continue;
				}

				// Buffers only requires global barriers.
				compiledPass.GetGlobalBarrier().srcAccess |= bufferState.subResourceState.previousState.access;
				compiledPass.GetGlobalBarrier().srcStage |= bufferState.subResourceState.previousState.stage;
				compiledPass.GetGlobalBarrier().dstAccess |= bufferState.subResourceState.state.access;
				compiledPass.GetGlobalBarrier().dstStage |= bufferState.subResourceState.state.stage;
			}

			for (const RGTextureState& textureState : pass->m_textureStates)
			{
				const RGTextureDesc& textureDesc = textureState.texture->GetDesc();

				RHI::ResourceBarrierInfo* activeBarrier = nullptr;

				uint32_t prevSubResourceIndex = 0;

				textureState.EnumerateSubResources([&](const RGSubResourceState& subResourceState, uint32_t subResourceIndex) 
				{
					if (!isBarrierRequired(subResourceState))
					{
						return;
					}

					// Check if a new barrier is required for some reason.
					if (!canMergeSubResourceBarriers(activeBarrier, subResourceState, subResourceIndex, prevSubResourceIndex))
					{
						if (isLayoutTransitionRequired(subResourceState))
						{
							uint32_t mipIndex, layerIndex, planeIndex;
							RHI::GetSubResourceFromIndex(subResourceIndex, textureDesc.mips, textureDesc.layers, mipIndex, layerIndex, planeIndex);

							activeBarrier = &compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, textureState.texture);
							activeBarrier->imageBarrier().subResource.baseMipLevel = mipIndex;
							activeBarrier->imageBarrier().subResource.baseArrayLayer = layerIndex;
							activeBarrier->imageBarrier().subResource.layerCount = 1;
							activeBarrier->imageBarrier().subResource.levelCount = 1;
						}
						else
						{
							activeBarrier = &compiledPass.GetGlobalBarrierInfo();
						}
					}

					if (activeBarrier->type == RHI::BarrierType::Image)
					{
						RHI::ImageBarrier& imageBarrier = activeBarrier->imageBarrier();
						imageBarrier.srcAccess |= subResourceState.previousState.access;
						imageBarrier.srcStage |= subResourceState.previousState.stage;
						imageBarrier.srcLayout |= subResourceState.previousState.layout;
						imageBarrier.dstAccess |= subResourceState.state.access;
						imageBarrier.dstStage |= subResourceState.state.stage;
						imageBarrier.dstLayout |= subResourceState.state.layout;
					}
					else
					{
						RHI::GlobalBarrier& globalBarrier = activeBarrier->globalBarrier();
						globalBarrier.srcAccess |= subResourceState.previousState.access;
						globalBarrier.srcStage |= subResourceState.previousState.stage;
						globalBarrier.dstAccess |= subResourceState.state.access;
						globalBarrier.dstStage |= subResourceState.state.stage;
					}

					prevSubResourceIndex = subResourceIndex;
				});
			}
		}
	}

	void RenderGraph::AssignExternalResourcesSrcState()
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::ResourceStateTracker> resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

		for (RGResourceRef resource : m_resources)
		{
			if (!resource->m_isExternal || resource->m_refCount == 0)
			{
				continue;
			}

			// If the resource is external the RHI resource has been assigned at this point.
			RefPtr<RHI::RHIResource> rhiResource = GetRHIResource(resource);

			if (resource->GetResourceType() == RGResourceType::Buffer)
			{
				RGBufferRef buffer = reinterpret_cast<RGBufferRef>(resource);
				const RHI::ResourceState& currentResourceState = resourceTracker->GetCurrentResourceState(rhiResource, 0);

				if (VT_CHECK(buffer->firstAccess))
				{
					buffer->firstAccess->previousState = currentResourceState;
				}
			}
			else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
			{
				RGUniformBufferRef uniformBuffer = reinterpret_cast<RGUniformBufferRef>(resource);
				const RHI::ResourceState& currentResourceState = resourceTracker->GetCurrentResourceState(rhiResource, 0);

				if (VT_CHECK(uniformBuffer->firstAccess))
				{
					uniformBuffer->firstAccess->previousState = currentResourceState;
				}
			}
			else if (resource->GetResourceType() == RGResourceType::Texture)
			{
				RGTextureRef texture = reinterpret_cast<RGTextureRef>(resource);

				EnumerateTextureSubResources(texture->firstAccess, [&resourceTracker, &rhiResource](RGSubResourceState& subResourceState, uint32_t subResourceIndex)
				{
					const RHI::ResourceState& currentResourceState = resourceTracker->GetCurrentResourceState(rhiResource, subResourceIndex);
					subResourceState.previousState = currentResourceState;
				});
			}
		}
	}

	void RenderGraph::TransitionExternalResource(RGBufferRef buffer)
	{
		if (!VT_CHECK(buffer->m_isExternal || buffer->m_isExtracted))
		{
			return;
		}

		if (buffer->GetRHIResource())
		{
			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			const RGResourceAccessState& subResourceAccessState = buffer->lastAccess;
			RGPassRef lastAccessPass = subResourceAccessState.pass;

			if (lastAccessPass)
			{
				RefPtr<RHI::RHIResource> rhiResource = buffer->GetRHIResource()->GetRHIBuffer();
				const RGSubResourceState& lastSubResourceState = lastAccessPass->m_bufferStates[subResourceAccessState.stateIndex].subResourceState;
				resourceTracker->TransitionResource(rhiResource, 0, lastSubResourceState.state.stage, lastSubResourceState.state.access, lastSubResourceState.state.layout);
			}
		}
	}

	void RenderGraph::TransitionExternalResource(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();

		if (!VT_CHECK(texture->m_isExternal || texture->m_isExtracted))
		{
			return;
		}

		if (texture->GetRHIResource())
		{
			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			RefPtr<RHI::RHIResource> rhiResource = texture->GetRHIResource()->GetRHITexture();

			for (uint32_t i = 0; i < texture->lastAccess.size(); ++i)
			{
				const RGResourceAccessState& subResourceAccessState = texture->lastAccess[i];
				RGPassRef lastAccessPass = subResourceAccessState.pass;

				if (lastAccessPass)
				{
					const RGSubResourceState* lastSubResourceState = lastAccessPass->m_textureStates[subResourceAccessState.stateIndex].subResourceStates[i];

					if (VT_CHECK(lastSubResourceState != nullptr))
					{
						resourceTracker->TransitionResource(rhiResource, i, lastSubResourceState->state.stage, lastSubResourceState->state.access, lastSubResourceState->state.layout);
					}
				}
			}
		}
	}

	void RenderGraph::TransitionExternalResource(RGUniformBufferRef buffer)
	{
		if (!VT_CHECK(buffer->m_isExternal || buffer->m_isExtracted))
		{
			return;
		}

		if (buffer->GetRHIResource())
		{
			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			const RGResourceAccessState& subResourceAccessState = buffer->lastAccess;
			RGPassRef lastAccessPass = subResourceAccessState.pass;

			if (lastAccessPass)
			{
				RefPtr<RHI::RHIResource> rhiResource = buffer->GetRHIResource()->GetRHIUniformBuffer();
				const RGSubResourceState& lastSubResourceState = lastAccessPass->m_bufferStates[subResourceAccessState.stateIndex].subResourceState;
				resourceTracker->TransitionResource(rhiResource, 0, lastSubResourceState.state.stage, lastSubResourceState.state.access, lastSubResourceState.state.layout);
			}
		}
	}

	void RenderGraph::ValidateTextureUAV(const RGTextureUAVDesc& uavDesc)
	{
		VT_ENSURE_MSG(uavDesc.textureResource->GetDesc().usage == RHI::ImageUsage::AttachmentStorage || uavDesc.textureResource->GetDesc().usage == RHI::ImageUsage::Storage, "Texture does not support UAVs!");
	}

	RGBufferSRVRef RenderGraph::CreateSRV(const RGBufferSRVDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!desc.bufferResource->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferSRVRef bufferSRV = m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
		m_resourceSRVs.emplace_back(bufferSRV);

		return bufferSRV;
	}

	RGUniformBufferSRVRef RenderGraph::CreateSRV(const RGUniformBufferSRVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		RGUniformBufferSRVRef bufferSRV = m_resourceAccessorAllocator.Allocate<RGUniformBufferSRV>(desc);
		m_resourceSRVs.emplace_back(bufferSRV);

		return bufferSRV;
	}

	RGBufferUAVRef RenderGraph::CreateUAV(const RGBufferUAVDesc& desc)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!desc.bufferResource->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferUAVRef bufferUAV = m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
		m_resourceUAVs.emplace_back(bufferUAV);

		return bufferUAV;
	}

	RGBufferSRVRef RenderGraph::CreateSRV(RGBufferRef buffer)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!buffer->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferSRVDesc desc{};
		desc.bufferResource = buffer;

		RGBufferSRVRef bufferSRV = m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
		m_resourceSRVs.emplace_back(bufferSRV);

		return bufferSRV;
	}

	RGBufferUAVRef RenderGraph::CreateUAV(RGBufferRef buffer)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!buffer->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferUAVDesc desc{};
		desc.bufferResource = buffer;

		RGBufferUAVRef bufferUAV = m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
		m_resourceUAVs.emplace_back(bufferUAV);

		return bufferUAV;
	}

	RGBufferSRVRef RenderGraph::CreateSRV(RGBufferRef buffer, RHI::PixelFormat format)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(buffer->GetDesc().isTexelBufferDesc, "Buffer must have been created as a texel buffer!");

		RGBufferSRVDesc desc{};
		desc.bufferResource = buffer;
		desc.format = format;

		RGBufferSRVRef bufferSRV = m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
		m_resourceSRVs.emplace_back(bufferSRV);

		return bufferSRV;
	}

	RGBufferUAVRef RenderGraph::CreateUAV(RGBufferRef buffer, RHI::PixelFormat format)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(buffer->GetDesc().isTexelBufferDesc, "Buffer must have been created as a texel buffer!");

		RGBufferUAVDesc desc{};
		desc.bufferResource = buffer;
		desc.format = format;

		RGBufferUAVRef bufferUAV = m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
		m_resourceUAVs.emplace_back(bufferUAV);

		return bufferUAV;
	}

	RGTextureSRVRef RenderGraph::CreateSRV(const RGTextureSRVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		RGTextureSRVRef textureSRV = m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
		m_resourceSRVs.emplace_back(textureSRV);
		return textureSRV;
	}

	RGTextureUAVRef RenderGraph::CreateUAV(const RGTextureUAVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		ValidateTextureUAV(desc);

		RGTextureUAVRef textureUAV = m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
		m_resourceUAVs.emplace_back(textureUAV);
		return textureUAV;
	}

	RGTextureSRVRef RenderGraph::CreateSRV(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(texture);

		RGTextureSRVDesc desc{};
		desc.textureResource = texture;

		RGTextureSRVRef textureSRV = m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
		m_resourceSRVs.emplace_back(textureSRV);
		return textureSRV;
	}

	RGTextureUAVRef RenderGraph::CreateUAV(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(texture);

		RGTextureUAVDesc desc{};
		desc.textureResource = texture;

		ValidateTextureUAV(desc);

		RGTextureUAVRef textureUAV = m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
		m_resourceUAVs.emplace_back(textureUAV);
		return textureUAV;
	}

	RGBufferRef RenderGraph::RegisterExternalBuffer(RefPtr<RHI::StorageBuffer> buffer)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(buffer);

		if (RGResourceRef resource = TryGetRegisteredExternalResource(buffer); resource != nullptr)
		{
			return reinterpret_cast<RGBufferRef>(resource);
		}

		const RHI::BufferDesc& rhiDesc = buffer->GetDesc();

		RGBufferDesc rgDesc;
		rgDesc.count = rhiDesc.count;
		rgDesc.elementSize = rhiDesc.elementSize;
		rgDesc.memoryUsage = rhiDesc.memoryUsage;
		rgDesc.usage = rhiDesc.usage;
		rgDesc.debugName = rhiDesc.debugName;
		rgDesc.isTexelBufferDesc = EnumValueContainsFlag(rhiDesc.usage, RHI::BufferUsage::TexelBuffer);

		RGBufferRef bufferResource = m_resourceAllocator.Allocate<RGBuffer>(rgDesc);
		bufferResource->m_isExternal = true;

		m_resources.emplace_back(bufferResource);
		m_resourceManager.AddExternalResource(bufferResource, buffer);

		RegisterExternalResource(buffer, bufferResource);

		return bufferResource;
	}

	RGUniformBufferRef RenderGraph::RegisterExternalUniformBuffer(RefPtr<RHI::UniformBuffer> uniformBuffer)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE(uniformBuffer);

		if (RGResourceRef resource = TryGetRegisteredExternalResource(uniformBuffer); resource != nullptr)
		{
			return reinterpret_cast<RGUniformBufferRef>(resource);
		}

		RGUniformBufferDesc desc{};
		desc.size = static_cast<uint32_t>(uniformBuffer->GetByteSize());
		desc.debugName = uniformBuffer->GetName();

		RGUniformBufferRef bufferResource = m_resourceAllocator.Allocate<RGUniformBuffer>(desc);
		bufferResource->m_isExternal = true;

		m_resources.emplace_back(bufferResource);
		m_resourceManager.AddExternalResource(bufferResource, uniformBuffer);

		RegisterExternalResource(uniformBuffer, bufferResource);

		return bufferResource;
	}

	RGTextureRef RenderGraph::RegisterExternalTexture(RefPtr<RHI::Image> texture)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE(texture);

		if (RGResourceRef resource = TryGetRegisteredExternalResource(texture); resource != nullptr)
		{
			return reinterpret_cast<RGTextureRef>(resource);
		}

		RGTextureDesc desc{};
		desc.width = texture->GetWidth();
		desc.height = texture->GetHeight();
		desc.depth = texture->GetDepth();
		desc.layers = texture->GetLayerCount();
		desc.mips = texture->GetMipCount();
		desc.format = texture->GetFormat();
		desc.usage = texture->GetUsage();
		desc.imageType = desc.depth > 1 ? RHI::ResourceType::Image3D : RHI::ResourceType::Image2D;
		desc.debugName = texture->GetName();
		desc.isCubeMap = texture->GetDesc().isCubeMap;

		RGTextureRef textureResource = m_resourceAllocator.Allocate<RGTexture>(desc);
		textureResource->m_isExternal = true;

		m_resources.emplace_back(textureResource);
		m_resourceManager.AddExternalResource(textureResource, texture);

		RegisterExternalResource(texture, textureResource);

		return textureResource;
	}

	void RenderGraph::EnqueueTextureExtraction(RGTextureRef texture, RefPtr<RHI::Image>* outImage)
	{
		texture->m_isExtracted = true;
		m_textureExtractions.emplace_back(texture, outImage);
	}

	void RenderGraph::EnqueueBufferExtraction(RGBufferRef buffer, RefPtr<RHI::StorageBuffer>* outBuffer)
	{
		buffer->m_isExtracted = true;
		m_bufferExtractions.emplace_back(buffer, outBuffer);
	}

	void RenderGraph::BeginMarker(const std::string& markerName, const glm::vec4& markerColor /*= 1.f*/)
	{
		m_standaloneMarkers.BeginMarker(static_cast<uint32_t>(m_renderPasses.size()), markerName, markerColor);
	}

	void RenderGraph::EndMarker()
	{
		m_standaloneMarkers.EndMarker(static_cast<uint32_t>(m_renderPasses.size()));
	}

	void RenderGraph::AddResourceBarrier(RGResourceRef resource, const RHI::ResourceState& barrierInfo)
	{
		VT_PROFILE_FUNCTION();

		const uint32_t passIndex = m_renderPasses.empty() ? 0u : static_cast<uint32_t>(m_renderPasses.size() - 1);

		auto& newBarrier = m_standaloneBarriers.AddBarrier(passIndex);
		newBarrier.resource = resource;
		newBarrier.type = resource->GetResourceType();
		newBarrier.newState = barrierInfo;
	}

	BEGIN_SHADER_PARAMETER_STRUCT(ReadbackBufferParameters)
		RG_BUFFER_ACCESS(SrcBuffer, RGResourceAccess::CopySrc)
		RG_BUFFER_ACCESS(DstBuffer, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	Ref<GPUReadbackBuffer> RenderGraph::EnqueueBufferReadback(RGBufferRef srcBuffer)
	{
		VT_PROFILE_FUNCTION();

		const size_t dataSize = srcBuffer->GetDesc().elementSize * srcBuffer->GetDesc().count;

		Ref<GPUReadbackBuffer> readbackBuffer = CreateRef<GPUReadbackBuffer>(dataSize);
		RGBufferRef dstBuffer = RegisterExternalBuffer(readbackBuffer->GetBuffer());

		RefPtr<RHI::Fence> fence = RHI::Fence::Create();

		ReadbackBufferParameters* parameters = AllocParameters<ReadbackBufferParameters>();
		parameters->SrcBuffer = srcBuffer;
		parameters->DstBuffer = dstBuffer;

		AddPass("Readback Copy Pass",
			RenderGraphPassFlags::None,
			parameters,
			[parameters, fence, dataSize, readbackBuffer](RenderContext& context)
		{
			context.CopyBufferRegion(parameters->SrcBuffer, 0, parameters->DstBuffer, 0, dataSize);
			context.Flush(fence);

			JobRef readbackJob = JobSystem::CreateJob("Readback", ExecutionPriority::Render, [fence, readbackBuffer]()
			{
				fence->WaitUntilSignaled();
				readbackBuffer->m_isReady = true;
			});
			JobSystem::RunJob(readbackJob);
		});

		return readbackBuffer;
	}

	BEGIN_SHADER_PARAMETER_STRUCT(ReadbackTextureParameters)
		RG_TEXTURE_ACCESS(SrcTexture, RGResourceAccess::CopySrc)
		RG_TEXTURE_ACCESS(DstTexture, RGResourceAccess::CopyDst)
	END_SHADER_PARAMETER_STRUCT()

	Ref<GPUReadbackTexture> RenderGraph::EnqueueTextureReadback(RGTextureRef srcTexture)
	{
		VT_PROFILE_FUNCTION();

		Ref<GPUReadbackTexture> readbackTexture = CreateRef<GPUReadbackTexture>(srcTexture->GetDesc());
		RGTextureRef dstTexture = RegisterExternalTexture(readbackTexture->GetImage());

		RefPtr<RHI::Fence> fence = RHI::Fence::Create();

		ReadbackTextureParameters* parameters = AllocParameters<ReadbackTextureParameters>();
		parameters->SrcTexture = srcTexture;
		parameters->DstTexture = dstTexture;

		AddPass("Readback Copy Pass",
			RenderGraphPassFlags::None,
			parameters,
			[parameters, fence, readbackTexture](RenderContext& context)
		{
			const auto& desc = parameters->SrcTexture->GetDesc();
			context.CopyTexture(parameters->SrcTexture, parameters->DstTexture, desc.width, desc.height, desc.depth);
			context.Flush(fence);

			JobRef readbackJob = JobSystem::CreateJob("Readback", ExecutionPriority::Render, [fence, readbackTexture]()
			{
				fence->WaitUntilSignaled();
				readbackTexture->m_isReady = true;
			});
			JobSystem::RunJob(readbackJob);
		});

		return readbackTexture;
	}

	void RenderGraph::Compile()
	{
		VT_PROFILE_FUNCTION();

		CullPasses();
		AssignExternalResourcesSrcState();
		BuildPassBarriers();
	}

	void RenderGraph::Execute()
	{
		ExecuteInternal(false, false, false);
	}

	void RenderGraph::ExecuteImmediate()
	{
		ExecuteInternal(true, false, false);
	}

	void RenderGraph::ExecuteImmediateAndWait()
	{
		ExecuteInternal(true, true, false);
	}

	JobCounterRef RenderGraph::ExecuteAndExtractCounter()
	{
		return ExecuteInternal(false, false, true);
	}

	JobCounterRef RenderGraph::ExecuteInternal(bool isImmediate, bool waitForSync, bool extractCounter)
	{
		VT_PROFILE_FUNCTION();

		if (m_renderPasses.empty())
		{
			return nullptr;
		}

		RenderGraphShaderParameterUniformBuffer* shaderParameterUniformBuffer = new RenderGraphShaderParameterUniformBuffer(*this);

		PrepareResourcesForExecution();
		CreateResourceViews();

		constexpr size_t NumPassesPerJob = 1;

		struct PassExecutionRange
		{
			uint16_t begin;
			uint16_t count;
		};

		Vector<PassExecutionRange> passExecutionRanges;
		passExecutionRanges.reserve(Math::DivideRoundUp(m_renderPasses.size(), NumPassesPerJob));

		size_t currentOffset = 0;
		while (currentOffset < m_renderPasses.size())
		{
			auto& newRange = passExecutionRanges.emplace_back();
			newRange.begin = static_cast<uint16_t>(currentOffset);
			newRange.count = static_cast<uint16_t>(std::min(NumPassesPerJob, (m_renderPasses.size() - currentOffset)));

			currentOffset += NumPassesPerJob;
		}

		// Each execution range gets their own command buffer.
		Vector<RefPtr<PooledCommandBuffer>> commandBuffers;
		commandBuffers.resize(passExecutionRanges.size());

		for (size_t i = 0; i < passExecutionRanges.size(); ++i)
		{
			commandBuffers[i] = CommandBufferPool::GetCommandBuffer();
		}

		shaderParameterUniformBuffer->Map();

		// Move this RenderGraph into temporary storage, so that the 
		// RenderGraph isn't destroyed before execution is finished.
		// This data pointer is destroyed in the execution job.

		// Create a temporary reference to the execution fence here to keep it alive.
		RefPtr<RHI::Fence> executionFence = m_executionFence;

		void* tempRenderGraphStorage = Memory::Malloc(sizeof(RenderGraph), alignof(RenderGraph));
		new (tempRenderGraphStorage) RenderGraph(std::move(*this));

		RenderGraph* renderGraphPtr = reinterpret_cast<RenderGraph*>(tempRenderGraphStorage);

		// This function executes the provided pass range.
		constexpr auto executePassRangeFunc = [](RenderGraph* renderGraphPtr, RenderGraphShaderParameterUniformBuffer& shaderParameterUniformBuffer, const PassExecutionRange& executionRange,
			const Vector<RefPtr<PooledCommandBuffer>>& commandBuffers, const uint32_t index, const uint32_t numExecutionRanges)
		{
			RefPtr<RHI::CommandBuffer> commandBuffer = commandBuffers.at(index)->Get();

			commandBuffer->Begin();

			if (index == 0)
			{
				commandBuffer->BeginMarker("RenderGraph::Execute", { 1.f, 1.f, 1.f, 1.f });
			}

			for (uint16_t i = executionRange.begin; i < executionRange.begin + executionRange.count; ++i)
			{
				RGPassRef pass = renderGraphPtr->m_renderPasses.at(i);
				const RGCompiledPass& compiledPass = renderGraphPtr->m_compiledRenderPasses.at(pass->passIndex);

				if (pass->m_isCulled)
				{
					renderGraphPtr->InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, commandBuffer);
					renderGraphPtr->InsertStandaloneMarkersIntoCommandBuffer(pass->passIndex, commandBuffer);
					continue;
				}

				renderGraphPtr->InsertStandaloneMarkersIntoCommandBuffer(pass->passIndex, commandBuffer);

				commandBuffer->BeginMarker(pass->m_name, { 1.f, 1.f, 1.f, 1.f });
				renderGraphPtr->InsertBarriersIntoCommandBuffer(compiledPass.prePassBarriers, commandBuffer);

				{
					VT_PROFILE_SCOPE(pass->m_name.data());
					RenderContext renderContext(*renderGraphPtr, pass, commandBuffer, shaderParameterUniformBuffer);
					renderGraphPtr->m_passAllocator.ExecutePass(pass, renderContext);
				}

				renderGraphPtr->InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, commandBuffer);
				commandBuffer->EndMarker();
			}

			if (index == numExecutionRanges - 1)
			{
				commandBuffer->EndMarker();
			}

			commandBuffer->End();
		};

		shaderParameterUniformBuffer->Unmap();

		// This function is responsible for executing the recorded command buffers.
		constexpr auto executeRenderGraphFunc = [](RenderGraph* renderGraphPtr, RenderGraphShaderParameterUniformBuffer* shaderParameterUniformBuffer, const Vector<RefPtr<PooledCommandBuffer>>& commandBuffers, RefPtr<RHI::Fence> executionFence)
		{
			RHI::DeviceQueueExecuteInfo executeInfo{};
			executeInfo.commandBuffers.resize(commandBuffers.size());

			for (size_t i = 0; i < commandBuffers.size(); ++i)
			{
				executeInfo.commandBuffers[i] = commandBuffers[i]->Get();
			}

			executeInfo.executionFence = executionFence;
			RHI::GraphicsContext::GetDevice()->GetDeviceQueue(RHI::QueueType::Graphics)->Execute(executeInfo);

			renderGraphPtr->TransitionExternalResources();
			renderGraphPtr->ExtractResources();

			// Destroy the RenderGraph.
			JobRef destroyJob = JobSystem::CreateJob("RenderGraph::Destroy", ExecutionPriority::Render, [renderGraphPtr, shaderParameterUniformBuffer]()
			{
				delete shaderParameterUniformBuffer;

				renderGraphPtr->~RenderGraph();
				Memory::Free(renderGraphPtr);
			});
			JobSystem::RunJob(destroyJob);
		};

		const uint32_t numExecutionRanges = static_cast<uint32_t>(passExecutionRanges.size());
		const bool isAsyncExecution = g_renderGraphForceSingleThreadedExecution.GetValue() == 0;

		JobCounterRef jobCounter = nullptr;

		if (isAsyncExecution)
		{
			TaskGraph taskGraph{ isImmediate ? ExecutionPriority::Immediate : ExecutionPriority::Render };
			Vector<TaskGraph::Task*> recordTasks(passExecutionRanges.size());

			for (uint32_t index = 0; const PassExecutionRange& executionRange : passExecutionRanges)
			{
				recordTasks[index] = taskGraph.AddTask("RenderGraph::Record", [renderGraphPtr, shaderParameterUniformBuffer, executionRange, commandBuffers, index, numExecutionRanges]()
				{
					executePassRangeFunc(renderGraphPtr, *shaderParameterUniformBuffer, executionRange, commandBuffers, index, numExecutionRanges);
				});
				index++;
			}

			taskGraph.AddTaskWithDependencies("RenderGraph::Execute", recordTasks, [renderGraphPtr, shaderParameterUniformBuffer, commandBuffers, executionFence]()
			{
				executeRenderGraphFunc(renderGraphPtr, shaderParameterUniformBuffer, commandBuffers, executionFence);
			});

			jobCounter = taskGraph.ExecuteAndExtractCounter();
		}
		else
		{
			for (uint32_t index = 0; const PassExecutionRange& executionRange : passExecutionRanges)
			{
				executePassRangeFunc(renderGraphPtr, *shaderParameterUniformBuffer, executionRange, commandBuffers, index, numExecutionRanges);
				index++;
			}

			executeRenderGraphFunc(renderGraphPtr, shaderParameterUniformBuffer, commandBuffers, executionFence);
		}

		if (waitForSync)
		{
			executionFence->WaitUntilSignaled();
		}

		if (isImmediate)
		{
			JobSystem::WaitForCounter(jobCounter);
		}

		if (!extractCounter)
		{
			// Remove the ref created from ExecuteAndExtractCounter here.
			JobSystem::DestroyCounter(jobCounter);
		}

		return jobCounter;
	}

	void RenderGraph::ExtractResources()
	{
		VT_PROFILE_FUNCTION();

		auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

		for (const auto& textureExtractionData : m_textureExtractions)
		{
			if (textureExtractionData.outImagePtr == nullptr)
			{
				continue;
			}

			*textureExtractionData.outImagePtr = textureExtractionData.texture->GetRHIResource()->GetRHITexture();

			// Update resource state of extracted texture
			TransitionExternalResource(textureExtractionData.texture);
		}

		for (const auto& bufferExtractionData : m_bufferExtractions)
		{
			if (bufferExtractionData.outBufferPtr == nullptr)
			{
				continue;
			}

			*bufferExtractionData.outBufferPtr = bufferExtractionData.buffer->GetRHIResource()->GetRHIBuffer();

			// Update resource state of extracted buffer
			TransitionExternalResource(bufferExtractionData.buffer);
		}
	}

	void RenderGraph::InsertBarriersIntoCommandBuffer(const RGCompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer)
	{
		VT_PROFILE_FUNCTION();

		if (passBarriers.Empty())
		{
			return;
		}

		const bool forceFullBarriersBetweenPasses = g_renderGraphForceFullBarriersBetweenPasses.GetValue() > 0;
		const size_t numBarriers = passBarriers.GetBarrierCount() + (forceFullBarriersBetweenPasses ? 1 : 0);

		RHI::BarrierVector resultBarriers;
		resultBarriers.reserve(numBarriers);

		for (const auto& passBarrier : passBarriers.GetBarriers())
		{
			VT_ENSURE(passBarrier.barrier.type == RHI::BarrierType::Global || passBarrier.resource != nullptr);

			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			auto& barrier = resultBarriers.emplace_back(passBarrier.barrier);
			if (barrier.type == RHI::BarrierType::Image)
			{
				barrier.imageBarrier().resource = GetRHIResource(passBarrier.resource);
			}
			else if (barrier.type == RHI::BarrierType::Buffer)
			{
				barrier.bufferBarrier().resource = GetRHIResource(passBarrier.resource);
			}
		}

		if (forceFullBarriersBetweenPasses)
		{
			auto& barrier = resultBarriers.emplace_back();
			barrier.type = RHI::BarrierType::Global;
			barrier.globalBarrier().srcStage |= RHI::BarrierStage::All;
			barrier.globalBarrier().srcAccess |= RHI::BarrierAccess::AllRead | RHI::BarrierAccess::AllWrite;
			barrier.globalBarrier().dstStage |= RHI::BarrierStage::All;
			barrier.globalBarrier().dstAccess |= RHI::BarrierAccess::AllRead | RHI::BarrierAccess::AllWrite;
		}

		commandBuffer->ResourceBarrier(resultBarriers);
	}

	void RenderGraph::InsertStandaloneMarkersIntoCommandBuffer(const uint32_t passIndex, const RefPtr<RHI::CommandBuffer>& commandBuffer)
	{
		VT_PROFILE_FUNCTION();

		if (m_standaloneMarkers.PassHasMarkers(passIndex))
		{
			for (const StandaloneMarkers::MarkerInfo& markerInfo : m_standaloneMarkers.GetMarkersForPassIndex(passIndex))
			{
				if (!markerInfo.isEnd)
				{
					commandBuffer->BeginMarker(markerInfo.markerName, { markerInfo.markerColor.x, markerInfo.markerColor.y, markerInfo.markerColor.z, markerInfo.markerColor.w });
				}
				else
				{
					commandBuffer->EndMarker();
				}
			}
		}
	}

	RGResourceRef RenderGraph::TryGetRegisteredExternalResource(RawPtr<RHI::RHIResource> resource)
	{
		if (m_registeredExternalResources.contains(resource))
		{
			return m_registeredExternalResources.at(resource);
		}

		return nullptr;
	}

	void RenderGraph::RegisterExternalResource(RawPtr<RHI::RHIResource> resource, RGResourceRef handle)
	{
		m_registeredExternalResources[resource] = handle;
	}

	RefPtr<Volt::RHI::RHIResource> RenderGraph::GetRHIResource(RGResourceRef resource)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<Volt::RHI::RHIResource> rhiResource;

		switch (resource->GetResourceType())
		{
			case RGResourceType::Texture:
			{
				RGTextureRef textureResource = reinterpret_cast<RGTextureRef>(resource);
				rhiResource = textureResource->GetRHIResource()->GetRHITexture();
				break;
			}

			case RGResourceType::Buffer:
			{
				RGBufferRef bufferResource = reinterpret_cast<RGBufferRef>(resource);
				rhiResource = bufferResource->GetRHIResource()->GetRHIBuffer();
				break;
			}

			case  RGResourceType::UniformBuffer:
			{
				RGUniformBufferRef bufferResource = reinterpret_cast<RGUniformBufferRef>(resource);
				rhiResource = bufferResource->GetRHIResource()->GetRHIUniformBuffer();
				break;
			}
		}

		return rhiResource;
	}

	RGSubResourceState* RenderGraph::AllocateSubResourceState()
	{
		// Allocate and call constructor
		RGSubResourceState* subResourceState = reinterpret_cast<RGSubResourceState*>(m_temporaryDataAllocator.Allocate(sizeof(RGSubResourceState)));
		new(subResourceState) RGSubResourceState();

		return subResourceState;
	}

	void RenderGraph::AddPassDependency(RGPassRef pass, RGResourceType resourceType, uint32_t subResourceIndex, RGSubResourceState& subResourceState, const RGResourceAccessState& lastAccess)
	{
		if (lastAccess.pass != nullptr)
		{
			if (pass->m_passDependencies.find(lastAccess.pass) == pass->m_passDependencies.end())
			{
				pass->m_passDependencies.emplace_back(lastAccess.pass);
				lastAccess.pass->m_refCount++;
			}

			if (resourceType == RGResourceType::Buffer || resourceType == RGResourceType::UniformBuffer)
			{
				const RGSubResourceState& prevSubResourceState = lastAccess.pass->m_bufferStates[lastAccess.stateIndex].subResourceState;
				subResourceState.previousState = prevSubResourceState.state;
			}
			else
			{
				const RGSubResourceState& prevSubResourceState = *lastAccess.pass->m_textureStates[lastAccess.stateIndex].subResourceStates[subResourceIndex];
				subResourceState.previousState = prevSubResourceState.state;
			}
		}
	}

	void RenderGraph::StandaloneMarkers::BeginMarker(uint32_t passIndex, const std::string& markerName, const glm::vec4& color)
	{
		auto& newMarker = m_markers[passIndex].emplace_back();
		newMarker.markerName = markerName;
		newMarker.markerColor = color;
		newMarker.isEnd = false;
	}

	void RenderGraph::StandaloneMarkers::EndMarker(uint32_t passIndex)
	{
		auto& newMarker = m_markers[passIndex].emplace_back();
		newMarker.isEnd = true;
	}

	RenderGraphShaderParameterUniformBuffer::RenderGraphShaderParameterUniformBuffer(RenderGraph& renderGraph)
		: m_head(0), m_mappedPtr(nullptr)
	{
		VT_PROFILE_FUNCTION();

		constexpr uint64_t TotalShaderParametersByteSize = 1 * 1024 * 1024;

		RGUniformBufferDesc desc{};
		desc.size = TotalShaderParametersByteSize;
		desc.debugName = "ShaderParameters";

		m_uniformBuffer = renderGraph.CreateUniformBuffer(desc);
		m_uniformBuffer->m_refCount++;

		// Create view
		RGUniformBufferSRVDesc srvDesc{};
		srvDesc.bufferResource = m_uniformBuffer;
		srvDesc.size = TotalShaderParametersByteSize;
		srvDesc.offset = 0;

		m_srv = renderGraph.CreateSRV(srvDesc);
	}

	void RenderGraphShaderParameterUniformBuffer::Map()
	{
		m_mappedPtr = m_uniformBuffer->GetRHIResource()->GetRHIUniformBuffer()->Map<void>();
	}

	void RenderGraphShaderParameterUniformBuffer::Unmap()
	{
		m_uniformBuffer->GetRHIResource()->GetRHIUniformBuffer()->Unmap();
	}

	uint64_t RenderGraphShaderParameterUniformBuffer::Allocate(uint64_t size)
	{
		const uint64_t alignedSize = size + g_rhiCapabilities.minUniformBufferAlignment;
		uint64_t allocOffset = m_head.fetch_add(alignedSize, std::memory_order::relaxed);
		return Utility::Align(allocOffset, g_rhiCapabilities.minUniformBufferAlignment);
	}
}
