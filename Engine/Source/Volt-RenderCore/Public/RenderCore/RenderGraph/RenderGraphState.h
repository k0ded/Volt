#pragma once

#include "RenderCore/RenderGraph/RenderGraphDataAllocator.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphTexture.h"

#include <RHIModule/Core/ResourceStateTracker.h>

#include <cstdint>

namespace Volt
{
	class RGTexture;
	class RGBuffer;
	class RGUniformBuffer;

	template<typename Func>
	void EnumerateSubResources(const RGTextureSubResourceState& subResourceStates, Func&& func);

	template<typename Func>
	void EnumerateSubResources(RGTextureSubResourceState& subResourceStates, Func&& func);

	struct RGTextureState
	{
		void Initialize(RGTexture* inTexture, RGResourceAccessType inAccessType);

		template<typename Func>
		void EnumerateSubResources(Func&& func);

		template<typename Func>
		void EnumerateSubResources(Func&& func) const;

		template<typename Func>
		void EnumerateSubResourceRange(const RGTextureSubResourceRange& subResourceRange, Func&& func);

		RGTexture* texture;
		RGTextureSubResourceState subResourceStates;
		uint32_t refCount;
		RGResourceAccessType accessType;
	};

	struct RGBufferState
	{
		void Initialize(RGBuffer* inBuffer, RGResourceAccessType inAccessType);
		void Initialize(RGUniformBuffer* inBuffer, RGResourceAccessType inAccessType);

		union
		{
			RGBuffer* buffer;
			RGUniformBuffer* uniformBuffer;
		};

		RGSubResourceState subResourceState;
		uint32_t refCount;
		RGResourceType bufferType;
		RGResourceAccessType accessType;
	};
}

#include "RenderCore/RenderGraph/RenderGraphState.inl"
