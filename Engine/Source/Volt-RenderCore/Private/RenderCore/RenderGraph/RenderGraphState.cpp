#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphState.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphBuffer.h"
#include "RenderCore/RenderGraph/RenderGraphAllocators.h"

#include <RHIModule/RHIHelpers.h>

namespace Volt
{
	RGSubResourceState::RGSubResourceState()
	{
		state.stage = RHI::BarrierStage::None;
		state.access = RHI::BarrierAccess::None;
		state.layout = RHI::ImageLayout::Undefined;

		previousState = state;
	}

	void RGTextureState::Initialize(RGTexture* inTexture, RGResourceAccessType inAccessType)
	{
		texture = inTexture;
		accessType = inAccessType;
		refCount = 0;

		const RGTextureDesc& desc = texture->GetDesc();

		const uint32_t numSubResources = desc.mips * desc.layers;
		subResourceStates.resize(numSubResources, nullptr);
	}

	void RGBufferState::Initialize(RGBuffer* inBuffer, RGResourceAccessType inAccessType)
	{
		buffer = inBuffer;
		bufferType = RGResourceType::Buffer;
		accessType = inAccessType;
		refCount = 0;
	}

	void RGBufferState::Initialize(RGUniformBuffer* inBuffer, RGResourceAccessType inAccessType)
	{
		uniformBuffer = inBuffer;
		bufferType = RGResourceType::UniformBuffer;
		accessType = inAccessType;
		refCount = 0;
	}

	void RGSubResourceState::AddState(RHI::BarrierStage stage, RHI::BarrierAccess access, RHI::ImageLayout layout)
	{
		// #TODO_Ivar: Add correct logic and validation.

		state.stage |= stage;
		state.access |= access;
		state.layout |= layout;
	}
}
