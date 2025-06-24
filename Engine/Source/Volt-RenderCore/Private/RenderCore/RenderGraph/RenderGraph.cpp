#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraph.h"
#include "RenderCore/RenderGraph/RenderContext.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"
#include "RenderCore/RenderGraph/RenderGraphExecutionThread.h"
#include "RenderCore/RenderGraph/GPUReadbackBuffer.h"
#include "RenderCore/RenderGraph/GPUReadbackTexture.h"

#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHIFeatures.h>

#include <JobSystem/JobSystem.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/ComparisonHelpers.h>

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

	inline bool GetIsWriteFromAccessMask(RHI::BarrierAccess access)
	{
		if (EnumValueContainsFlag(access, RHI::BarrierAccess::ShaderWrite) ||
			EnumValueContainsFlag(access, RHI::BarrierAccess::DepthStencilWrite) ||
			EnumValueContainsFlag(access, RHI::BarrierAccess::CopyDest) ||
			EnumValueContainsFlag(access, RHI::BarrierAccess::VideoEncodeWrite) ||
			EnumValueContainsFlag(access, RHI::BarrierAccess::VideoDecodeWrite))
		{
			return true;
		}

		return false;
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

	RenderGraph::RenderGraph(RefPtr<RHI::CommandBuffer> commandBuffer)
		: m_commandBuffer(commandBuffer)
	{
		VT_PROFILE_FUNCTION();

		RHI::FenceCreateInfo createInfo{};
		m_executionFence = RHI::Fence::Create(createInfo);
	}

	RenderGraph::~RenderGraph()
	{
	}

	RenderGraph::RenderGraph(RenderGraph&& other) noexcept
		: m_transientResourceSystem(std::move(other.m_transientResourceSystem)),
		m_registeredExternalResources(std::move(other.m_registeredExternalResources)),
		m_resourceAllocator(std::move(other.m_resourceAllocator)),
		m_resourceAccessorAllocator(std::move(other.m_resourceAccessorAllocator)),
		m_passParametersAllocator(std::move(other.m_passParametersAllocator)),
		m_passAllocator(std::move(other.m_passAllocator)),
		m_passes(std::move(other.m_passes)),
		m_resources(std::move(other.m_resources)),
		m_compiledPasses(std::move(other.m_compiledPasses)),
		m_commandBuffer(std::move(other.m_commandBuffer)),
		m_executionFence(std::move(other.m_executionFence)),
		m_textureExtractions(std::move(other.m_textureExtractions)),
		m_bufferExtractions(std::move(other.m_bufferExtractions)),
		m_standaloneBarriers(std::move(other.m_standaloneBarriers)),
		m_standaloneMarkers(std::move(other.m_standaloneMarkers)),
		m_temporaryDataAllocator(std::move(other.m_temporaryDataAllocator)),
		m_resourceStateTracker(std::move(other.m_resourceStateTracker))
	{
	}

	RenderGraph& RenderGraph::operator=(RenderGraph&& other) noexcept
	{
		if (this == &other)
		{
			return *this;
		}

		m_transientResourceSystem = std::move(other.m_transientResourceSystem);
		m_registeredExternalResources = std::move(other.m_registeredExternalResources);
		m_resourceAllocator = std::move(other.m_resourceAllocator);
		m_resourceAccessorAllocator = std::move(other.m_resourceAccessorAllocator);
		m_passParametersAllocator = std::move(other.m_passParametersAllocator);
		m_passAllocator = std::move(other.m_passAllocator);
		m_passes = std::move(other.m_passes);
		m_resources = std::move(other.m_resources);
		m_compiledPasses = std::move(other.m_compiledPasses);
		m_commandBuffer = std::move(other.m_commandBuffer);
		m_executionFence = std::move(other.m_executionFence);
		m_textureExtractions = std::move(other.m_textureExtractions);
		m_bufferExtractions = std::move(other.m_bufferExtractions);
		m_standaloneBarriers = std::move(other.m_standaloneBarriers);
		m_standaloneMarkers = std::move(other.m_standaloneMarkers);
		m_temporaryDataAllocator = std::move(other.m_temporaryDataAllocator);
		m_resourceStateTracker = std::move(other.m_resourceStateTracker);

		return *this;
	}

	RGBuffer* RenderGraph::CreateBuffer(const RGBufferDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		RGBufferRef buffer = m_resourceAllocator.Allocate<RGBuffer>(desc);
		m_resources.emplace_back(buffer);

		return buffer;
	}

	RGTexture* RenderGraph::CreateTexture(const RGTextureDesc& desc)
	{
		VT_PROFILE_FUNCTION();

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
			if (resource->isExternal)
			{
				RefPtr<RHI::RHIResource> rhiResource;

				if (resource->GetResourceType() == RGResourceType::Texture)
				{
					rhiResource = m_transientResourceSystem.GetTextureIfExists(reinterpret_cast<RGTextureRef>(resource));
				}
				else if (resource->GetResourceType() == RGResourceType::Buffer)
				{
					rhiResource = m_transientResourceSystem.GetBufferIfExists(reinterpret_cast<RGBufferRef>(resource));
				}
				else if (resource->GetResourceType() == RGResourceType::UniformBuffer)
				{
					rhiResource = m_transientResourceSystem.GetUniformBufferIfExists(reinterpret_cast<RGUniformBufferRef>(resource));
				}

				const RGResourceState& resourceState = m_resourceStateTracker.GetState(resource);
				resourceTracker->TransitionResource(rhiResource, resourceState.currentState.stage, resourceState.currentState.access, resourceState.currentState.layout);
			}
		}
	}

	RGBufferSRVRef RenderGraph::CreateSRV(const RGBufferSRVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(!desc.bufferResource->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");
		return m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
	}

	RGUniformBufferSRVRef RenderGraph::CreateSRV(RGUniformBufferRef uniformBuffer)
	{
		VT_PROFILE_FUNCTION();

		return m_resourceAccessorAllocator.Allocate<RGUniformBufferSRV>(uniformBuffer);
	}

	RGBufferUAVRef RenderGraph::CreateUAV(const RGBufferUAVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(!desc.bufferResource->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");
		return m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
	}

	RGBufferSRVRef RenderGraph::CreateSRV(RGBufferRef buffer)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(!buffer->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferSRVDesc desc{};
		desc.bufferResource = buffer;
		return m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
	}

	RGBufferUAVRef RenderGraph::CreateUAV(RGBufferRef buffer)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(!buffer->GetDesc().isTexelBufferDesc, "Buffer format has to be provided if the buffer is a texel buffer!");

		RGBufferUAVDesc desc{};
		desc.bufferResource = buffer;
		return m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
	}

	RGBufferSRVRef RenderGraph::CreateSRV(RGBufferRef buffer, RHI::PixelFormat format)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(buffer->GetDesc().isTexelBufferDesc, "Buffer must have been created as a texel buffer!");

		RGBufferSRVDesc desc{};
		desc.bufferResource = buffer;
		desc.format = format;
		return m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
	}

	RGBufferUAVRef RenderGraph::CreateUAV(RGBufferRef buffer, RHI::PixelFormat format)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(buffer->GetDesc().isTexelBufferDesc, "Buffer must have been created as a texel buffer!");

		RGBufferUAVDesc desc{};
		desc.bufferResource = buffer;
		desc.format = format;
		return m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
	}

	RGTextureSRVRef RenderGraph::CreateSRV(const RGTextureSRVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		return m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
	}

	RGTextureUAVRef RenderGraph::CreateUAV(const RGTextureUAVDesc& desc)
	{
		VT_PROFILE_FUNCTION();

		return m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
	}

	RGTextureSRVRef RenderGraph::CreateSRV(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(texture);

		RGTextureSRVDesc desc{};
		desc.textureResource = texture;
		return m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
	}

	RGTextureUAVRef RenderGraph::CreateUAV(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(texture);

		RGTextureUAVDesc desc{};
		desc.textureResource = texture;
		return m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
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
		bufferResource->isExternal = true;

		m_resources.emplace_back(bufferResource);
		m_transientResourceSystem.AddExternalResource(bufferResource, buffer);

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
		desc.elementSize = uniformBuffer->GetByteSize();
		desc.name = uniformBuffer->GetName();

		RGUniformBufferRef bufferResource = m_resourceAllocator.Allocate<RGUniformBuffer>(desc);
		bufferResource->isExternal = true;

		m_resources.emplace_back(bufferResource);
		m_transientResourceSystem.AddExternalResource(bufferResource, uniformBuffer);

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
		textureResource->isExternal = true;

		m_resources.emplace_back(textureResource);
		m_transientResourceSystem.AddExternalResource(textureResource, texture);

		RegisterExternalResource(texture, textureResource);

		return textureResource;
	}

	void RenderGraph::EnqueueTextureExtraction(RGTextureRef texture, RefPtr<RHI::Image>* outImage)
	{
		texture->isExtracted = true;
		m_textureExtractions.emplace_back(texture, outImage);
	}

	void RenderGraph::EnqueueBufferExtraction(RGBufferRef buffer, RefPtr<RHI::StorageBuffer>* outBuffer)
	{
		buffer->isExtracted = true;
		m_bufferExtractions.emplace_back(buffer, outBuffer);
	}

	void RenderGraph::BeginMarker(const std::string& markerName, const glm::vec4& markerColor /*= 1.f*/)
	{
		m_standaloneMarkers.BeginMarker(static_cast<uint32_t>(m_passes.size()), markerName, markerColor);
	}

	void RenderGraph::EndMarker()
	{
		m_standaloneMarkers.EndMarker(static_cast<uint32_t>(m_passes.size()));
	}

	void RenderGraph::AddResourceBarrier(RGResourceRef resource, const RHI::ResourceState& barrierInfo)
	{
		VT_PROFILE_FUNCTION();

		const uint32_t passIndex = m_passes.empty() ? 0u : static_cast<uint32_t>(m_passes.size() - 1);

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

		RefPtr<RHI::Fence> fence = RHI::Fence::Create(RHI::FenceCreateInfo{ false });

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

			JobRef readbackJob = JobSystem::CreateJob("Readback", ExecutionPriority::Immediate, [fence, readbackBuffer]()
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

		RefPtr<RHI::Fence> fence = RHI::Fence::Create(RHI::FenceCreateInfo{ false });

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

		m_compiledPasses.resize(m_passes.size());

		///// Calculate Ref Count //////
		for (auto pass : m_passes)
		{
			pass->refCount = static_cast<uint32_t>(pass->GetResourceWrites().size() + pass->GetResourceRenderTargetAccesses().size());

			for (auto resource : pass->GetResourceReads())
			{
				resource->GetResource()->AddRef();
			}

			// Mark the first writer of a resource as it's producer
			for (auto resource : pass->GetResourceWrites())
			{
				if (!resource->GetResource()->HasProducer(resource))
				{
					resource->GetResource()->AddProducer(pass, resource);
					// #TODO_Ivar: Leaves this here for now, reference: RenderGraphCullingTests::WriteAfterWrite
					//pass->refCount++;
				}
				else
				{
					resource->GetResource()->AddRef();
				}
			}

			for (auto resource : pass->GetResourceRenderTargetAccesses())
			{
				if (!resource->HasProducer())
				{
					resource->AddProducer(pass);

					// If this pass is the render targets producer, we need to increase the ref count of the pass.
					// #TODO_Ivar: Leaves this here for now, reference: RenderGraphCullingTests::WriteAfterWrite
					//pass->refCount++;
				}
				else if (!resource->IsProducer(pass))
				{
					// We add a reference if we are not the producer 
					// of this resource, because we can then consider it being a "read"
					resource->AddRef();
				}
			}

			for (auto resourceAccess : pass->GetResourceAccesses())
			{
				// Copy Dst can be seen as a "produce" operation, as it puts data
				// into the resource.
				if (resourceAccess.accessType == RGResourceAccess::CopyDst)
				{
					if (!resourceAccess.resource->HasProducer())
					{
						resourceAccess.resource->AddProducer(pass);
					}

					// We need to increase the ref count of the pass as well.
					pass->refCount++;
				}
			}
		}

		///// Cull Passes /////
		for (auto pass : m_passes)
		{
			// If a pass has no references, it doesn't have any output.
			// In this case all reads should have it's references removed.
			// And then it should be marked as culled.
			if (pass->refCount == 0 && !EnumValueContainsFlag(pass->flags, RenderGraphPassFlags::NeverCull))
			{
				for (auto resource : pass->GetResourceReads())
				{
					resource->GetResource()->DecRef();
				}

				// All non produced writes as well
				for (auto resource : pass->GetResourceWrites())
				{
					if (!resource->GetResource()->IsProducer(pass))
					{
						resource->GetResource()->DecRef();
					}
				}

				// And all non producer render target accesses
				for (auto resource : pass->GetResourceRenderTargetAccesses())
				{
					if (!resource->IsProducer(pass))
					{
						resource->DecRef();
					}
				}

				pass->isCulled = true;
			}
		}

		Vector<RGResourceRef> unreferencedResources{};
		for (auto node : m_resources)
		{
			if (node->GetRefCount() == 0)
			{
				unreferencedResources.emplace_back(node);
			}
		}

		while (!unreferencedResources.empty())
		{
			RGResourceRef unreferencedResource = unreferencedResources.back();
			unreferencedResources.pop_back();

			// If the resource is external, or queued to be extracted, we will skip 
			// the culling logic.
			if (unreferencedResource->isExternal || unreferencedResource->isExtracted)
			{
				continue;
			}

			for (const Handle<RenderGraphPass> producer : unreferencedResource->producers)
			{
				// If the pass has been marked as never cull, we won't continue this iteration
				if (EnumValueContainsFlag(producer->flags, RenderGraphPassFlags::NeverCull))
				{
					continue;
				}

				VT_ENSURE_MSG(producer->refCount > 0, "Ref count cannot be zero at this time!");

				// Decrease the reference counter on the producer, and then decrease the reference counter of it's resource reads.
				// This might produce more unreferenced resources and continue the loop.
				producer->refCount--;
				if (producer->refCount == 0)
				{
					for (auto resourceAccess : producer->GetResourceReads())
					{
						auto resource = resourceAccess->GetResource();
						resource->DecRef();

						if (resource->GetRefCount() == 0)
						{
							unreferencedResources.emplace_back(resource);
						}
					}

					for (auto resource : producer->GetResourceRenderTargetAccesses())
					{
						if (!resource->IsProducer(producer))
						{
							resource->DecRef();
							if (resource->GetRefCount() == 0)
							{
								unreferencedResources.emplace_back(resource);
							}
						}
					}

					producer->isCulled = true;
				}
			}
		}

		///// Find last resource usages /////
		for (auto pass : m_passes)
		{
			if (pass->isCulled)
			{
				continue;
			}

			for (auto resourceAccess : pass->GetResourceReads())
			{
				resourceAccess->GetResource()->lastUser = pass;
			}

			for (auto resourceAccess : pass->GetResourceWrites())
			{
				resourceAccess->GetResource()->lastUser = pass;
			}

			for (auto resourceAccess : pass->GetResourceRenderTargetAccesses())
			{
				resourceAccess->lastUser = pass;
			}
		}

		///// Find surrenderable resources /////
		for (auto resource : m_resources)
		{
			// If the resource doesn't have any last user it will not be used.
			if (!resource->lastUser || resource->isExternal)
			{
				continue;
			}

			const uint32_t passIndex = resource->lastUser->passIndex;
			m_compiledPasses[passIndex].AddSurrenderableResource(resource);
		}

		// Add all external resources to the resource state tracker
		for (auto resource : m_resources)
		{
			// Skip all non-external resources
			if (!resource->isExternal)
			{
				continue;
			}

			const RGResourceType resourceType = resource->GetResourceType();
			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			if (resourceType == RGResourceType::Texture)
			{
				const RHI::ResourceState& resourceState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetTextureIfExists(reinterpret_cast<RGTextureRef>(resource)));

				RGResourceState& currentState = m_resourceStateTracker.GetState(resource);
				currentState.currentState = resourceState;

				if (EnumValueContainsAnyFlag(resourceState.access, RHI::BarrierAccess::DepthStencilWrite, RHI::BarrierAccess::ShaderWrite, RHI::BarrierAccess::RenderTarget))
				{
					currentState.isWriteState = true;
				}
			}
			else if (resourceType == RGResourceType::Buffer)
			{
				const RHI::ResourceState& resourceState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetBufferIfExists(reinterpret_cast<RGBufferRef>(resource)));

				RGResourceState& currentState = m_resourceStateTracker.GetState(resource);
				currentState.currentState = resourceState;

				if (EnumValueContainsAnyFlag(resourceState.access, RHI::BarrierAccess::ShaderWrite))
				{
					currentState.isWriteState = true;
				}
			}
			else if (resourceType == RGResourceType::UniformBuffer)
			{
				m_resourceStateTracker.GetState(resource).currentState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetUniformBufferIfExists(reinterpret_cast<RGUniformBufferRef>(resource)));
			}
		}

		///// Setup Barriers /////
		for (auto pass : m_passes)
		{
			auto& compiledPass = m_compiledPasses.at(pass->passIndex);
			compiledPass.SetName(pass->name);

			if (!pass->isCulled)
			{
				for (auto resourceAccess : pass->GetResourceWrites())
				{
					const RGResourceRef resource = resourceAccess->GetResource();
					const RGResourceType resourceType = resource->GetResourceType();

					// We start by figuring out the state that we want to take the resource to.
					RHI::ResourceState newState{};

					// Compute shader.
					if (EnumValueContainsFlag(pass->flags, RenderGraphPassFlags::Compute))
					{
						// For compute shaders, all the resource types have the same accesses
						newState.access = RHI::BarrierAccess::ShaderWrite;
						newState.stage = RHI::BarrierStage::ComputeShader;
						newState.layout = RHI::ImageLayout::ShaderWrite;
					}
					else
					{
						if (IsEqualToAny(resourceType, RGResourceType::Buffer, RGResourceType::UniformBuffer))
						{
							newState.access = RHI::BarrierAccess::ShaderWrite;
							newState.stage = RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader;
							newState.layout = RHI::ImageLayout::ShaderWrite;

							if (RHI::RHICanUseMeshShaders())
							{
								newState.stage |= RHI::BarrierStage::MeshShader | RHI::BarrierStage::AmplificationShader;
							}
						}
					}

					// Handle cases
					if (!resource->IsFirstProducer(pass))
					{
						auto& resourceState = m_resourceStateTracker.GetState(resource);

						const bool isSameLayoutType = IsEqualToAny(resourceType, RGResourceType::Texture) ? newState.layout == resourceState.currentState.layout : true;
						const bool isBufferType = IsEqualToAny(resourceType, RGResourceType::Buffer, RGResourceType::UniformBuffer);

						// Handle cases 2, 4, 5
						if (isBufferType || isSameLayoutType)
						{
							compiledPass.GetGlobalBarrier().srcAccess |= resourceState.currentState.access;
							compiledPass.GetGlobalBarrier().srcStage |= resourceState.currentState.stage;
							compiledPass.GetGlobalBarrier().dstAccess |= newState.access;
							compiledPass.GetGlobalBarrier().dstStage |= newState.stage;
						}
						// It's not a buffer and the image needs to transition layout, handle case 9.
						else
						{
							const bool requireExternalSrcAccess = resource->isExternal && resourceState.previousUsage == nullptr;

							auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource, requireExternalSrcAccess);
							newBarrier.imageBarrier().srcAccess = resourceState.currentState.access;
							newBarrier.imageBarrier().srcStage = resourceState.currentState.stage;
							newBarrier.imageBarrier().srcLayout = resourceState.currentState.layout;
							newBarrier.imageBarrier().dstAccess = newState.access;
							newBarrier.imageBarrier().dstStage = newState.stage;
							newBarrier.imageBarrier().dstLayout = newState.layout;
						}

						resourceState.currentState = newState;
						resourceState.isWriteState = true;
						resourceState.previousUsage = pass;
					}
					// If the pass is this resources producer, we handle it a little bit different because this will be the first entry
					// in the resource state tracker.
					else
					{
						VT_ENSURE(resourceType != RGResourceType::UniformBuffer);

						auto& resourceState = m_resourceStateTracker.GetState(resource);
						resourceState.currentState = newState;
						resourceState.previousUsage = pass;
						resourceState.isWriteState = true;

						if (IsEqualToAny(resourceType, RGResourceType::Texture))
						{
							auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource);
							newBarrier.imageBarrier().dstAccess = newState.access;
							newBarrier.imageBarrier().dstStage = newState.stage;
							newBarrier.imageBarrier().dstLayout = newState.layout;
						}
						else if (resourceType == RGResourceType::Buffer)
						{
							compiledPass.GetGlobalBarrier().dstAccess |= newState.access;
							compiledPass.GetGlobalBarrier().dstStage |= newState.stage;
						}
					}
				}

				for (auto resource : pass->GetResourceRenderTargetAccesses())
				{
					// For textures there are a couple of cases to consider.
					// It could be a color image, or a depth image, and needs
					// to be setup accordingly.
					RHI::ResourceState newState = GetWriteStateForRasterizedTexture(resource);

					if (!resource->IsFirstProducer(pass))
					{
						auto& resourceState = m_resourceStateTracker.GetState(resource);

						const bool isSameLayoutType = newState.layout == resourceState.currentState.layout;
					
						// If it's the same layout (read after read / write after write) then we only need a global barrier.
						if (isSameLayoutType)
						{
							compiledPass.GetGlobalBarrier().srcAccess |= resourceState.currentState.access;
							compiledPass.GetGlobalBarrier().srcStage |= resourceState.currentState.stage;
							compiledPass.GetGlobalBarrier().dstAccess |= newState.access;
							compiledPass.GetGlobalBarrier().dstStage |= newState.stage;
						}
						else
						{
							const bool requireExternalSrcAccess = resource->isExternal && resourceState.previousUsage == nullptr;

							auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource, requireExternalSrcAccess);
							newBarrier.imageBarrier().srcAccess = resourceState.currentState.access;
							newBarrier.imageBarrier().srcStage = resourceState.currentState.stage;
							newBarrier.imageBarrier().srcLayout = resourceState.currentState.layout;
							newBarrier.imageBarrier().dstAccess = newState.access;
							newBarrier.imageBarrier().dstStage = newState.stage;
							newBarrier.imageBarrier().dstLayout = newState.layout;
						}

						resourceState.currentState = newState;
						resourceState.isWriteState = true;
						resourceState.previousUsage = pass;
					}
					// If the pass is this resources producer, we handle it a little bit different because this will be the first entry
					// in the resource state tracker.
					else
					{
						auto& resourceState = m_resourceStateTracker.GetState(resource);
						resourceState.currentState = newState;
						resourceState.previousUsage = pass;
						resourceState.isWriteState = true;

						auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource);
						newBarrier.imageBarrier().dstAccess = newState.access;
						newBarrier.imageBarrier().dstStage = newState.stage;
						newBarrier.imageBarrier().dstLayout = newState.layout;
					}
				}

				for (auto resourceAccess : pass->GetResourceReads())
				{
					const RGResourceRef resource = resourceAccess->GetResource();
					const RGResourceType resourceType = resource->GetResourceType();

					// We start by figuring out the state that we want to take the resource to.
					RHI::ResourceState newState{};

					// When the resource is being read, the access and layout is the same
					// for both compute and rasterization passes.
					newState.access = RHI::BarrierAccess::ShaderRead;
					newState.layout = RHI::ImageLayout::ShaderRead;

					if (EnumValueContainsFlag(RenderGraphPassFlags::Compute, pass->flags))
					{
						newState.stage = RHI::BarrierStage::ComputeShader;
					}
					else
					{
						newState.stage = RHI::BarrierStage::VertexShader | RHI::BarrierStage::PixelShader;

						if (RHI::RHICanUseMeshShaders())
						{
							newState.stage |= RHI::BarrierStage::MeshShader | RHI::BarrierStage::AmplificationShader;
						}
					}

					// Handle cases
					auto& resourceState = m_resourceStateTracker.GetState(resource);

					// Handle case 1 and 3
					if (!resourceState.isWriteState)
					{
						// As both the previous and the current access are read operations, no barrier is required.
						continue;
					}

					const bool isBufferType = IsEqualToAny(resourceType, RGResourceType::Buffer, RGResourceType::UniformBuffer);

					// Handle case 8
					if (isBufferType)
					{
						compiledPass.GetGlobalBarrier().srcAccess |= resourceState.currentState.access;
						compiledPass.GetGlobalBarrier().srcStage |= resourceState.currentState.stage;
						compiledPass.GetGlobalBarrier().dstAccess |= newState.access;
						compiledPass.GetGlobalBarrier().dstStage |= newState.stage;
					}
					// If we reach this point, it's an image that needs a layout transition.
					else
					{
						const bool requireExternalSrcAccess = resource->isExternal && resourceState.previousUsage == nullptr;

						auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource, requireExternalSrcAccess);
						newBarrier.imageBarrier().srcAccess = resourceState.currentState.access;
						newBarrier.imageBarrier().srcStage = resourceState.currentState.stage;
						newBarrier.imageBarrier().srcLayout = resourceState.currentState.layout;
						newBarrier.imageBarrier().dstAccess = newState.access;
						newBarrier.imageBarrier().dstStage = newState.stage;
						newBarrier.imageBarrier().dstLayout = newState.layout;
					}

					resourceState.currentState = newState;
					resourceState.isWriteState = false;
					resourceState.previousUsage = pass;
				}

				for (auto resourceAccess : pass->GetResourceAccesses())
				{
					VT_ENSURE(resourceAccess.accessType != RGResourceAccess::None);

					const RGResourceRef resource = resourceAccess.resource;
					const RGResourceType resourceType = resource->GetResourceType();

					RHI::ResourceState newState{};
					SetupResourceStateFromAccess(resourceAccess.accessType, newState);

					// Handle cases
					auto& resourceState = m_resourceStateTracker.GetState(resource);

					// Handle case 1 and 3
					if (!resourceState.isWriteState)
					{
						// As both the previous and the current access are read operations, no barrier is required.
						continue;
					}

					const bool isBufferType = IsEqualToAny(resourceType, RGResourceType::Buffer, RGResourceType::UniformBuffer);

					// Handle case 8
					if (isBufferType)
					{
						compiledPass.GetGlobalBarrier().srcAccess |= resourceState.currentState.access;
						compiledPass.GetGlobalBarrier().srcStage |= resourceState.currentState.stage;
						compiledPass.GetGlobalBarrier().dstAccess |= newState.access;
						compiledPass.GetGlobalBarrier().dstStage |= newState.stage;
					}
					// If we reach this point, it's an image that needs a layout transition.
					else
					{
						const bool requireExternalSrcAccess = resource->isExternal && resourceState.previousUsage == nullptr;

						auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource, requireExternalSrcAccess);
						newBarrier.imageBarrier().srcAccess = resourceState.currentState.access;
						newBarrier.imageBarrier().srcStage = resourceState.currentState.stage;
						newBarrier.imageBarrier().srcLayout = resourceState.currentState.layout;
						newBarrier.imageBarrier().dstAccess = newState.access;
						newBarrier.imageBarrier().dstStage = newState.stage;
						newBarrier.imageBarrier().dstLayout = newState.layout;
					}

					resourceState.currentState = newState;
					resourceState.isWriteState = false;
					resourceState.previousUsage = pass;
				}
			}

			// Handle standalone barriers
			if (m_standaloneBarriers.HasPassBarriers(pass->passIndex))
			{
				for (const auto& barrier : m_standaloneBarriers.GetPassBarriers(pass->passIndex))
				{
					auto& resourceState = m_resourceStateTracker.GetState(barrier.resource);

					const bool isSameLayoutType = IsEqualToAny(barrier.type, RGResourceType::Texture) ? barrier.newState.layout == resourceState.currentState.layout : true;
					const bool isBufferType = IsEqualToAny(barrier.type, RGResourceType::Buffer, RGResourceType::UniformBuffer);

					if (isBufferType || isSameLayoutType)
					{
						compiledPass.GetPostPassGlobalBarrier().srcAccess |= resourceState.currentState.access;
						compiledPass.GetPostPassGlobalBarrier().srcStage |= resourceState.currentState.stage;
						compiledPass.GetPostPassGlobalBarrier().dstAccess |= barrier.newState.access;
						compiledPass.GetPostPassGlobalBarrier().dstStage |= barrier.newState.stage;
					}
					// It's not a buffer and the image needs to transition layout, handle case 9.
					else
					{
						const bool requireExternalSrcAccess = barrier.resource->isExternal && resourceState.previousUsage == nullptr;

						auto& newBarrier = compiledPass.postPassBarriers.AddBarrier(RHI::BarrierType::Image, barrier.resource, requireExternalSrcAccess);
						newBarrier.imageBarrier().srcAccess = resourceState.currentState.access;
						newBarrier.imageBarrier().srcStage = resourceState.currentState.stage;
						newBarrier.imageBarrier().srcLayout = resourceState.currentState.layout;
						newBarrier.imageBarrier().dstAccess = barrier.newState.access;
						newBarrier.imageBarrier().dstStage = barrier.newState.stage;
						newBarrier.imageBarrier().dstLayout = barrier.newState.layout;
					}

					resourceState.currentState = barrier.newState;
					resourceState.isWriteState = GetIsWriteFromAccessMask(barrier.newState.access);
					resourceState.previousUsage = pass;
				}
			}
		}
	}

	void RenderGraph::Execute()
	{
		RenderGraphExecutionThread::ExecuteRenderGraph(std::move(*this));
	}

	void RenderGraph::ExecuteImmediate()
	{
		ExecuteInternal(false);
	}

	void RenderGraph::ExecuteImmediateAndWait()
	{
		ExecuteInternal(true);
	}

	void RenderGraph::ExecuteInternal(bool waitForSync)
	{
		VT_PROFILE_FUNCTION();

		m_commandBuffer->Begin();
		m_commandBuffer->BeginMarker("RenderGraph::Execute", { 1.f, 1.f, 1.f, 1.f });
		for (auto pass : m_passes)
		{
			const CompiledPass& compiledPass = m_compiledPasses.at(pass->passIndex);

			if (pass->isCulled)
			{
				InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, m_commandBuffer);
				InsertStandaloneMarkersIntoCommandBuffer(pass->passIndex, m_commandBuffer);
				continue;
			}

			InsertStandaloneMarkersIntoCommandBuffer(pass->passIndex, m_commandBuffer);
			
			m_commandBuffer->BeginMarker(pass->name, { 1.f, 1.f, 1.f, 1.f });
			InsertBarriersIntoCommandBuffer(compiledPass.prePassBarriers, m_commandBuffer);

			{
				VT_PROFILE_SCOPE(pass->name.data());
				RenderContext renderContext(*this, pass.GetRaw(), m_commandBuffer);
				m_passAllocator.ExecutePass(pass, renderContext);
			}

			InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, m_commandBuffer);
			m_commandBuffer->EndMarker();

			for (const RGResourceRef resource : compiledPass.GetSurrenderableResources())
			{
				// #TODO_Ivar: This doesn't work correctly yet.
				m_transientResourceSystem.SurrenderResource(resource, 0);
			}
		}

		// Make sure markers added after the final pass also are added.
		if (m_standaloneMarkers.PassHasMarkers(static_cast<uint32_t>(m_passes.size())))
		{
			InsertStandaloneMarkersIntoCommandBuffer(static_cast<uint32_t>(m_passes.size()), m_commandBuffer);
		}

		m_commandBuffer->EndMarker();
		m_commandBuffer->End();
		m_commandBuffer->ExecuteWithFence(m_executionFence);

		if (waitForSync)
		{
			m_executionFence->WaitUntilSignaled();
		}

		TransitionExternalResources();
		ExtractResources();
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

			*textureExtractionData.outImagePtr = m_transientResourceSystem.GetTextureIfExists(textureExtractionData.texture);

			// Update resource state of extracted texture
			const RGResourceState& resourceState = m_resourceStateTracker.GetState(textureExtractionData.texture);
			resourceTracker->TransitionResource(*textureExtractionData.outImagePtr, resourceState.currentState.stage, resourceState.currentState.access, resourceState.currentState.layout);
		}

		for (const auto& bufferExtractionData : m_bufferExtractions)
		{
			if (bufferExtractionData.outBufferPtr == nullptr)
			{
				continue;
			}

			*bufferExtractionData.outBufferPtr = m_transientResourceSystem.GetBufferIfExists(bufferExtractionData.buffer);

			// Update resource state of extracted buffer
			const RGResourceState& resourceState = m_resourceStateTracker.GetState(bufferExtractionData.buffer);
			resourceTracker->TransitionResource(*bufferExtractionData.outBufferPtr, resourceState.currentState.stage, resourceState.currentState.access);
		}
	}

	void RenderGraph::InsertBarriersIntoCommandBuffer(const CompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer)
	{
		VT_PROFILE_FUNCTION();

		if (passBarriers.Empty())
		{
			return;
		}

		RHI::BarrierVector resultBarriers;
		resultBarriers.reserve(passBarriers.GetBarrierCount());

		for (const auto& passBarrier : passBarriers.GetBarriers())
		{
			VT_ENSURE(passBarrier.barrier.type == RHI::BarrierType::Global || passBarrier.resource != nullptr);

			auto resourceTracker = RHI::GraphicsContext::GetResourceStateTracker();

			auto& barrier = resultBarriers.emplace_back(passBarrier.barrier);
			if (barrier.type == RHI::BarrierType::Image)
			{
				auto resource = GetRHIResource(passBarrier.resource);

				if (passBarrier.requiresExternalSrcState)
				{
					const RHI::ResourceState& prevResourceState = resourceTracker->GetCurrentResourceState(resource);
					barrier.imageBarrier().srcStage = prevResourceState.stage;
					barrier.imageBarrier().srcAccess = prevResourceState.access;
					barrier.imageBarrier().srcLayout = prevResourceState.layout;
				}

				barrier.imageBarrier().resource = resource;
			}
			else if (barrier.type == RHI::BarrierType::Buffer)
			{
				barrier.bufferBarrier().resource = GetRHIResource(passBarrier.resource);
			}
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

	RefPtr<RHI::BufferView> RenderGraph::GetRHIBufferSRV(RGBufferSRVRef bufferSRV)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::StorageBuffer> rhiBuffer = m_transientResourceSystem.AcquireBuffer(reinterpret_cast<RGBufferRef>(bufferSRV->GetResource()));

		RHI::BufferViewDesc desc{};
		desc.bufferFormat = bufferSRV->GetDesc().format;

		return rhiBuffer->GetView(desc);
	}

	RefPtr<RHI::BufferView> RenderGraph::GetRHIBufferUAV(RGBufferUAVRef bufferUAV)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::StorageBuffer> rhiBuffer = m_transientResourceSystem.AcquireBuffer(reinterpret_cast<RGBufferRef>(bufferUAV->GetResource()));

		RHI::BufferViewDesc desc{};
		desc.bufferFormat = bufferUAV->GetDesc().format;

		return rhiBuffer->GetView(desc);
	}

	RefPtr<RHI::ImageView> RenderGraph::GetRHITextureSRV(RGTextureSRVRef textureSRV)
	{
		VT_PROFILE_FUNCTION();

		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureSRV->GetResource());
		const RGTextureDesc& textureDesc = texture->GetDesc();
		const RGTextureSRVDesc& srvDesc = textureSRV->GetDesc();

		RHI::ImageViewDesc viewDesc{};
		viewDesc.baseMipLevel = srvDesc.baseMipLevel;
		viewDesc.baseArrayLayer = srvDesc.baseArrayLayer;
		viewDesc.mipCount = srvDesc.mipCount;
		viewDesc.layerCount = srvDesc.layerCount;

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

		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(texture);
		return rhiImage->GetView(viewDesc);
	}

	RefPtr<RHI::ImageView> RenderGraph::GetRHITextureUAV(RGTextureUAVRef textureUAV)
	{
		VT_PROFILE_FUNCTION();

		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureUAV->GetResource());
		const RGTextureDesc& textureDesc = texture->GetDesc();
		const RGTextureUAVDesc& uavDesc = textureUAV->GetDesc();

		RHI::ImageViewDesc viewDesc{};
		viewDesc.baseMipLevel = uavDesc.baseMipLevel;
		viewDesc.baseArrayLayer = uavDesc.baseArrayLayer;
		viewDesc.mipCount = uavDesc.mipCount;
		viewDesc.layerCount = uavDesc.layerCount;

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
				viewDesc.viewType = RHI::ImageViewType::View2DArray;
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

		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(texture);
		return rhiImage->GetView(viewDesc);
	}

	RefPtr<Volt::RHI::ImageView> RenderGraph::GetRHITextureRT(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(texture);
		return rhiImage->GetView();
	}

	RefPtr<Volt::RHI::RHIResource> RenderGraph::GetRHIResource(RGResourceRef resource)
	{
		VT_PROFILE_FUNCTION();

		RefPtr<Volt::RHI::RHIResource> rhiResource;

		switch (resource->GetResourceType())
		{
			case RGResourceType::Texture:
			{
				rhiResource = m_transientResourceSystem.AcquireTexture(reinterpret_cast<RGTextureRef>(resource));
				break;
			}

			case RGResourceType::Buffer:
			{
				rhiResource = m_transientResourceSystem.AcquireBuffer(reinterpret_cast<RGBufferRef>(resource));
				break;
			}

			case  RGResourceType::UniformBuffer:
			{
				rhiResource = m_transientResourceSystem.AcquireUniformBuffer(reinterpret_cast<RGUniformBufferRef>(resource));
				break;
			}
		}

		return rhiResource;
	}

	RefPtr<RHI::StorageBuffer> RenderGraph::GetRHIBuffer(RGBufferRef buffer)
	{
		VT_PROFILE_FUNCTION();

		return m_transientResourceSystem.AcquireBuffer(buffer);
	}

	RefPtr<RHI::UniformBuffer> RenderGraph::GetRHIUniformBuffer(RGUniformBufferRef uniformBuffer)
	{
		VT_PROFILE_FUNCTION();

		return m_transientResourceSystem.AcquireUniformBuffer(uniformBuffer);
	}

	RefPtr<RHI::Image> RenderGraph::GetRHITexture(RGTextureRef texture)
	{
		VT_PROFILE_FUNCTION();

		return m_transientResourceSystem.AcquireTexture(texture);
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
}
