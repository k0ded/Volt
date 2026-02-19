#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphPass.h"

namespace Volt
{
	RGTextureState& RGPass::GetOrCreateTextureState(RGTextureSRVRef textureSRV)
	{
		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureSRV->GetResource());
		return GetOrCreateTextureState(texture, RGResourceAccessType::Read);
	}

	RGTextureState& RGPass::GetOrCreateTextureState(RGTextureUAVRef textureUAV)
	{
		RGTextureRef texture = reinterpret_cast<RGTextureRef>(textureUAV->GetResource());
		return GetOrCreateTextureState(texture, RGResourceAccessType::Write);
	}
	
	RGTextureState& RGPass::GetOrCreateTextureState(RGTextureRef texture, RGResourceAccessType accessType)
	{
		RGTextureState* statePtr = nullptr;

		if (texture->firstPassAccessor == nullptr)
		{
			texture->firstPassAccessor = this;
		}

		for (RGTextureState& state : m_textureStates)
		{
			if (state.texture == texture)
			{
				statePtr = &state;
				break;
			}
		}

		if (!statePtr)
		{
			RGTextureState& newState = m_textureStates.emplace_back();
			newState.Initialize(texture, accessType, m_dataAllocator);

			statePtr = &newState;
		}

		VT_ENSURE(statePtr != nullptr);

		statePtr->refCount++;
		return *statePtr;
	}

	RGBufferState& RGPass::GetOrCreateBufferState(RGBufferSRVRef bufferSRV)
	{
		RGBufferRef buffer = reinterpret_cast<RGBufferRef>(bufferSRV->GetResource());
		return GetOrCreateBufferState(buffer, RGResourceAccessType::Read);
	}
	
	RGBufferState& RGPass::GetOrCreateBufferState(RGBufferUAVRef bufferUAV)
	{
		RGBufferRef buffer = reinterpret_cast<RGBufferRef>(bufferUAV->GetResource());
		return GetOrCreateBufferState(buffer, RGResourceAccessType::Write);
	}

	RGBufferState& RGPass::GetOrCreateBufferState(RGBufferRef buffer, RGResourceAccessType accessType)
	{
		RGBufferState* statePtr = nullptr;

		if (buffer->firstPassAccessor == nullptr)
		{
			buffer->firstPassAccessor = this;
		}

		for (RGBufferState& state : m_bufferStates)
		{
			if (state.bufferType == RGResourceType::Buffer && state.buffer == buffer)
			{
				statePtr = &state;
				break;
			}
		}

		if (!statePtr)
		{
			RGBufferState& newState = m_bufferStates.emplace_back();
			newState.Initialize(buffer, accessType);

			statePtr = &newState;
		}

		VT_ENSURE(statePtr != nullptr);

		statePtr->refCount++;
		return *statePtr;
	}

	RGBufferState& RGPass::GetOrCreateBufferState(RGUniformBufferRef buffer)
	{
		RGBufferState* statePtr = nullptr;

		if (buffer->firstPassAccessor == nullptr)
		{
			buffer->firstPassAccessor = this;
		}

		for (RGBufferState& state : m_bufferStates)
		{
			if (state.bufferType == RGResourceType::UniformBuffer && state.uniformBuffer == buffer)
			{
				statePtr = &state;
				break;
			}
		}

		if (!statePtr)
		{
			RGBufferState& newState = m_bufferStates.emplace_back();
			newState.Initialize(buffer, RGResourceAccessType::Read);

			statePtr = &newState;
		}

		VT_ENSURE(statePtr != nullptr);

		statePtr->refCount++;
		return *statePtr;
	}

	void RGPass::SetupAllocators(RenderGraphDataAllocator* dataAllocator)
	{
		m_textureStates.set_allocator({ dataAllocator });
		m_bufferStates.set_allocator({ dataAllocator });
		m_passDependencies.set_allocator({ dataAllocator });
	}
}
