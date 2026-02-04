#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"
#include "RenderCore/RenderGraph/RenderGraphParameterStruct.h"

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

		template<typename T>
		RenderGraphPass(const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata)
			: passParameters(shaderParameters, shaderParameterMetadata)
		{}

		std::string name;
		void* passAllocationStartPtr;
		uint32_t passIndex = 0;
		uint32_t refCount = 0;
		bool isCulled = false;
		RenderGraphPassFlags flags = RenderGraphPassFlags::None;

		RenderGraphParameterStruct passParameters;

		VT_INLINE void AddResourceRead(RGBufferSRVRef bufferSRV) { VT_ENSURE_MSG(bufferSRV != nullptr, "Must be a valid SRV!"); m_resourceReads.emplace_back(bufferSRV); }
		VT_INLINE void AddResourceRead(RGTextureSRVRef textureSRV) { VT_ENSURE_MSG(textureSRV != nullptr, "Must be a valid SRV!"); m_resourceReads.emplace_back(textureSRV); }
		VT_INLINE void AddResourceRead(RGUniformBufferSRVRef uniformBufferSRV) { VT_ENSURE_MSG(uniformBufferSRV != nullptr, "Must be a valid SRV!"); m_resourceReads.emplace_back(uniformBufferSRV); }

		VT_INLINE void AddResourceWrite(RGBufferUAVRef bufferUAV) { VT_ENSURE_MSG(bufferUAV != nullptr, "Must be a valid UAV!"); m_resourceWrites.emplace_back(bufferUAV); }
		VT_INLINE void AddResourceWrite(RGTextureUAVRef textureUAV) { VT_ENSURE_MSG(textureUAV != nullptr, "Must be a valid UAV!"); m_resourceWrites.emplace_back(textureUAV); }

		VT_INLINE void AddResourceRenderTargetAccess(RGTextureRef texture) { VT_ENSURE_MSG(texture != nullptr, "Must be a valid texture!"); m_renderTargetAccesses.emplace_back(texture); }

		VT_INLINE void AddResourceAccess(RGBufferRef buffer, RGResourceAccess accessType) { VT_ENSURE_MSG(buffer != nullptr, "Must be a valid buffer!"); m_resourceAccesses.emplace_back(buffer, accessType); }
		VT_INLINE void AddResourceAccess(RGTextureRef texture, RGResourceAccess accessType) { VT_ENSURE_MSG(texture != nullptr, "Must be a valid texture!"); m_resourceAccesses.emplace_back(texture, accessType); }
		VT_INLINE void AddResourceAccess(RGUniformBufferRef uniformBuffer, RGResourceAccess accessType) { VT_ENSURE_MSG(uniformBuffer != nullptr, "Must be a valid uniform buffer!"); m_resourceAccesses.emplace_back(uniformBuffer, accessType); }

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

	using RenderGraphPassRef = RenderGraphPass*;
}
