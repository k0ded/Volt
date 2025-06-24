#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <CoreUtilities/Containers/VectorVariants.h>

#include <string>

namespace Volt
{
	enum class RenderGraphPassFlags
	{
		None = 0,
		NeverCull = BIT(0),
		Compute = BIT(1)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(RenderGraphPassFlags);

	class RenderGraphPass
	{ 
	public:
		struct ResourceAccess
		{
			RGResourceRef resource;
			RGResourceAccess accessType;
		};

		std::string name;
		void* passAllocationStartPtr;
		uint32_t passIndex = 0;
		uint32_t refCount = 0;
		bool isCulled = false;
		RenderGraphPassFlags flags = RenderGraphPassFlags::None;

		VT_INLINE void AddResourceRead(RGBufferSRVRef bufferSRV) { m_resourceReads.emplace_back(bufferSRV); }
		VT_INLINE void AddResourceRead(RGTextureSRVRef textureSRV) { m_resourceReads.emplace_back(textureSRV); }
		VT_INLINE void AddResourceRead(RGUniformBufferSRVRef uniformBufferSRV) { m_resourceReads.emplace_back(uniformBufferSRV); }

		VT_INLINE void AddResourceWrite(RGBufferUAVRef bufferUAV) { m_resourceWrites.emplace_back(bufferUAV); }
		VT_INLINE void AddResourceWrite(RGTextureUAVRef textureUAV) { m_resourceWrites.emplace_back(textureUAV); }

		VT_INLINE void AddResourceRenderTargetAccess(RGTextureRef texture) { m_renderTargetAccesses.emplace_back(texture); }

		VT_INLINE void AddResourceAccess(RGBufferRef buffer, RGResourceAccess accessType) { m_resourceAccesses.emplace_back(buffer, accessType); }
		VT_INLINE void AddResourceAccess(RGTextureRef texture, RGResourceAccess accessType) { m_resourceAccesses.emplace_back(texture, accessType); }
		VT_INLINE void AddResourceAccess(RGUniformBufferRef uniformBuffer, RGResourceAccess accessType) { m_resourceAccesses.emplace_back(uniformBuffer, accessType); }

		VT_NODISCARD VT_INLINE const Vector<RGResourceSRVRef>& GetResourceReads() const { return m_resourceReads; }
		VT_NODISCARD VT_INLINE const Vector<RGResourceUAVRef>& GetResourceWrites() const { return m_resourceWrites; }
		VT_NODISCARD VT_INLINE const Vector<ResourceAccess>& GetResourceAccesses() const { return m_resourceAccesses; }
		VT_NODISCARD VT_INLINE const Vector<RGTextureRef>& GetResourceRenderTargetAccesses() const { return m_renderTargetAccesses; }

	private:
		Vector<RGResourceSRVRef> m_resourceReads;
		Vector<RGResourceUAVRef> m_resourceWrites;
		Vector<RGTextureRef> m_renderTargetAccesses;
		Vector<ResourceAccess> m_resourceAccesses;
	};
}
