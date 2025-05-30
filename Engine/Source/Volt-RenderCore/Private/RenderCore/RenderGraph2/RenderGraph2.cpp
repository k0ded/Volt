#include "rcpch.h"

#include "RenderCore/RenderGraph2/RenderGraph2.h"
#include "RenderCore/RenderGraph2/RenderContext2.h"
#include "RenderCore/RenderGraph/RenderGraphCommon.h"

#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/Images/ImageUtility.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHIFeatures.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/ComparisonHelpers.h>

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
	}

	RenderGraph2::RenderGraph2(RefPtr<RHI::CommandBuffer> commandBuffer)
		: m_commandBuffer(commandBuffer)
	{
		RHI::FenceCreateInfo createInfo{};
		m_executionFence = RHI::Fence::Create(createInfo);
	}

	RenderGraph2::~RenderGraph2()
	{
	}

	RGBuffer* RenderGraph2::CreateBuffer(const RGBufferDesc& desc)
	{
		RGBufferRef buffer = m_resourceAllocator.Allocate<RGBuffer>(desc);
		m_resources.emplace_back(buffer);

		return buffer;
	}

	RGTexture* RenderGraph2::CreateTexture(const RGTextureDesc& desc)
	{
		RGTextureRef texture = m_resourceAllocator.Allocate<RGTexture>(desc);
		m_resources.emplace_back(texture);

		return texture;
	}

	RGUniformBufferRef RenderGraph2::CreateUniformBuffer(const RGUniformBufferDesc& desc)
	{
		RGUniformBufferRef uniformBuffer = m_resourceAllocator.Allocate<RGUniformBuffer>(desc);
		m_resources.emplace_back(uniformBuffer);

		return uniformBuffer;
	}

	RGBufferSRVRef RenderGraph2::CreateSRV(const RGBufferSRVDesc& desc)
	{
		return m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
	}
	
	RGBufferUAVRef RenderGraph2::CreateUAV(const RGBufferUAVDesc& desc)
	{
		return m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
	}

	RGBufferSRVRef RenderGraph2::CreateSRV(RGBufferRef buffer)
	{
		RGBufferSRVDesc desc{};
		desc.bufferResource = buffer;
		return m_resourceAccessorAllocator.Allocate<RGBufferSRV>(desc);
	}

	RGBufferUAVRef RenderGraph2::CreateUAV(RGBufferRef buffer)
	{
		RGBufferUAVDesc desc{};
		desc.bufferResource = buffer;
		return m_resourceAccessorAllocator.Allocate<RGBufferUAV>(desc);
	}
	
	RGTextureSRVRef RenderGraph2::CreateSRV(const RGTextureSRVDesc& desc)
	{
		return m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
	}
	
	RGTextureUAVRef RenderGraph2::CreateUAV(const RGTextureUAVDesc& desc)
	{
		return m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
	}

	RGTextureSRVRef RenderGraph2::CreateSRV(RGTextureRef texture)
	{
		RGTextureSRVDesc desc{};
		desc.textureResource = texture;
		return m_resourceAccessorAllocator.Allocate<RGTextureSRV>(desc);
	}

	RGTextureUAVRef RenderGraph2::CreateUAV(RGTextureRef texture)
	{
		RGTextureUAVDesc desc{};
		desc.textureResource = texture;
		return m_resourceAccessorAllocator.Allocate<RGTextureUAV>(desc);
	}

	RGBufferRef RenderGraph2::RegisterExternalBuffer(RefPtr<RHI::StorageBuffer> buffer)
	{
		VT_ENSURE(buffer);

		if (RGResourceRef resource = TryGetRegisteredExternalResource(buffer); resource != nullptr)
		{
			return reinterpret_cast<RGBufferRef>(resource);
		}

		RGBufferDesc desc{};
		desc.name = buffer->GetName();
		desc.count = buffer->GetCount();
		desc.elementSize = buffer->GetElementSize();

		RGBufferRef bufferResource = m_resourceAllocator.Allocate<RGBuffer>(desc);
		bufferResource->isExternal = true;

		m_resources.emplace_back(bufferResource);
		m_transientResourceSystem.AddExternalResource(bufferResource, buffer);

		RegisterExternalResource(buffer, bufferResource);

		return bufferResource;
	}

	RGUniformBufferRef RenderGraph2::RegisterExternalUniformBuffer(RefPtr<RHI::UniformBuffer> uniformBuffer)
	{
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

	RGTextureRef RenderGraph2::RegisterExternalTexture(RefPtr<RHI::Image> texture)
	{
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

		RGTextureRef textureResource = m_resourceAllocator.Allocate<RGTexture>(desc);
		textureResource->isExternal = true;

		m_resources.emplace_back(textureResource);
		m_transientResourceSystem.AddExternalResource(textureResource, texture);

		RegisterExternalResource(texture, textureResource);

		return textureResource;
	}

	void RenderGraph2::Compile()
	{
		VT_PROFILE_FUNCTION();

		m_compiledPasses.resize(m_passes.size());

		///// Calculate Ref Count //////
		for (auto pass : m_passes)
		{
			pass->refCount = static_cast<uint32_t>(pass->GetResourceWrites().size()); // #TODO_Ivar: Assign correct value

			for (auto resource : pass->GetResourceReads())
			{
				resource->GetResource()->AddRef();
			}

			// Mark the first writer of a resource as it's producer
			for (auto resource : pass->GetResourceWrites())
			{
				if (!resource->GetResource()->producer)
				{
					resource->GetResource()->producer = pass;
					resource->GetResource()->isProduced = true;
				}
			}

			for (auto resource : pass->GetResourceRenderTargetAccesses())
			{
				if (!resource->producer)
				{
					resource->producer = pass;
					resource->isProduced = true;
				}
			}
		}

		///// Cull Passes /////
		PagedVector<RGResourceRef> unreferencedResources{};
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

			if (unreferencedResource->isExternal)
			{
				continue;
			}

			auto producer = unreferencedResource->producer;
			VT_ENSURE_MSG(producer, "Node should always have a producer!");

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

				producer->isCulled = true;
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

		struct ResourceState
		{
			Handle<RenderGraphPass> previousUsage;
			RHI::ResourceState currentState;
			bool isWriteState = false;
		};

		struct RGResourceStateTracker
		{
			inline ResourceState& GetState(RGResourceRef resource) { return resourceStates[resource]; }
			vt::map<RGResourceRef, ResourceState> resourceStates;

		} resourceStateTracker;
	
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
				resourceStateTracker.GetState(resource).currentState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetTextureIfExists(reinterpret_cast<RGTextureRef>(resource)));
			}
			else if (resourceType == RGResourceType::Buffer)
			{
				resourceStateTracker.GetState(resource).currentState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetBufferIfExists(reinterpret_cast<RGBufferRef>(resource)));
			}
			else if (resourceType == RGResourceType::UniformBuffer)
			{
				resourceStateTracker.GetState(resource).currentState = resourceTracker->GetCurrentResourceState(m_transientResourceSystem.GetUniformBufferIfExists(reinterpret_cast<RGUniformBufferRef>(resource)));
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
					if (pass != resource->producer)
					{
						auto& resourceState = resourceStateTracker.GetState(resource);

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
							auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource);
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

						auto& resourceState = resourceStateTracker.GetState(resource);
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

					// If the pass is this resources producer, we handle it a little bit different because this will be the first entry
					// in the resource state tracker.
					if (resource->producer == pass)
					{
						auto& resourceState = resourceStateTracker.GetState(resource);
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
					auto& resourceState = resourceStateTracker.GetState(resource);

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
						auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource);
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
					auto& resourceState = resourceStateTracker.GetState(resource);

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
						auto& newBarrier = compiledPass.prePassBarriers.AddBarrier(RHI::BarrierType::Image, resource);
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
		}
	}

	void RenderGraph2::Execute()
	{
		ExecuteInternal();
	}

	void RenderGraph2::ExecuteInternal()
	{
		AllocateShaderParametersBuffer();

		m_sharedRenderContext.SetRenderGraphConstantsBuffer(m_shaderParametersUniformBuffer);
		m_sharedRenderContext.BeginContext();

		m_commandBuffer->Begin();
		for (uint32_t passIndex = 0; auto pass : m_passes)
		{
			const CompiledPass& compiledPass = m_compiledPasses.at(passIndex);

			if (pass->isCulled)
			{
				InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, m_commandBuffer);
				continue;
			}

			m_commandBuffer->BeginMarker(pass->name, { 1.f, 1.f, 1.f, 1.f });

			InsertBarriersIntoCommandBuffer(compiledPass.prePassBarriers, m_commandBuffer);

			{
				VT_PROFILE_SCOPE(pass->name.data());
				RenderContext2 renderContext(*this, m_sharedRenderContext, pass.GetRaw(), m_commandBuffer);
				m_passAllocator.ExecutePass(pass, renderContext);
			}

			InsertBarriersIntoCommandBuffer(compiledPass.postPassBarriers, m_commandBuffer);

			m_commandBuffer->EndMarker();

			for (const RGResourceRef resource : compiledPass.GetSurrenderableResources())
			{
				// #TODO_Ivar: This doesn't work correctly yet.
				m_transientResourceSystem.SurrenderResource(resource, 0);
			}

			passIndex++;
		}
		m_commandBuffer->End();

		m_sharedRenderContext.EndContext();

		m_commandBuffer->ExecuteWithFence(m_executionFence);
	}

	void RenderGraph2::AllocateShaderParametersBuffer()
	{
		RGUniformBufferDesc desc{};
		desc.count = std::max(m_passAllocator.GetNumPasses(), 1u);
		desc.elementSize = RenderGraphCommon::MAX_PASS_CONSTANTS_SIZE;
		desc.name = "ShaderParameters";

		RGUniformBufferRef uniformBuffer = CreateUniformBuffer(desc);
		m_shaderParametersUniformBuffer = m_transientResourceSystem.AcquireUniformBuffer(uniformBuffer);
	}

	void RenderGraph2::InsertBarriersIntoCommandBuffer(const CompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer)
	{
		if (passBarriers.Empty())
		{
			return;
		}

		Vector<RHI::ResourceBarrierInfo> resultBarriers;
		resultBarriers.reserve(passBarriers.GetBarrierCount());

		for (const auto& passBarrier : passBarriers.GetBarriers())
		{
			VT_ENSURE(passBarrier.barrier.type == RHI::BarrierType::Global || passBarrier.resource != nullptr);

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

		commandBuffer->ResourceBarrier(resultBarriers);
	}

	RGResourceRef RenderGraph2::TryGetRegisteredExternalResource(RawPtr<RHI::RHIResource> resource)
	{
		if (m_registeredExternalResources.contains(resource))
		{
			return m_registeredExternalResources.at(resource);
		}

		return nullptr;
	}
	
	void RenderGraph2::RegisterExternalResource(RawPtr<RHI::RHIResource> resource, RGResourceRef handle)
	{
		m_registeredExternalResources[resource] = handle;
	}

	RefPtr<RHI::BufferView> RenderGraph2::GetRHIBufferSRV(RGBufferSRVRef bufferSRV)
	{
		RefPtr<RHI::StorageBuffer> rhiBuffer = m_transientResourceSystem.AcquireBuffer(reinterpret_cast<RGBufferRef>(bufferSRV->GetResource()));
		return rhiBuffer->GetView();
	}
	
	RefPtr<RHI::BufferView> RenderGraph2::GetRHIBufferUAV(RGBufferUAVRef bufferUAV)
	{
		RefPtr<RHI::StorageBuffer> rhiBuffer = m_transientResourceSystem.AcquireBuffer(reinterpret_cast<RGBufferRef>(bufferUAV->GetResource()));
		return rhiBuffer->GetView();
	}

	RefPtr<RHI::ImageView> RenderGraph2::GetRHITextureSRV(RGTextureSRVRef textureSRV)
	{
		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(reinterpret_cast<RGTextureRef>(textureSRV->GetResource()));
		return rhiImage->GetView();
	}

	RefPtr<RHI::ImageView> RenderGraph2::GetRHITextureUAV(RGTextureUAVRef textureUAV)
	{
		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(reinterpret_cast<RGTextureRef>(textureUAV->GetResource()));
		return rhiImage->GetView();
	}

	RefPtr<Volt::RHI::ImageView> RenderGraph2::GetRHITextureRT(RGTextureRef texture)
	{
		RefPtr<RHI::Image> rhiImage = m_transientResourceSystem.AcquireTexture(texture);
		return rhiImage->GetView();
	}

	RefPtr<Volt::RHI::RHIResource> RenderGraph2::GetRHIResource(RGResourceRef resource)
	{
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
}
