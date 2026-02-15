#pragma once

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"
#include "RenderCore/RenderGraph/RenderGraphParameterStruct.h"
#include "RenderCore/RenderGraph/RenderGraphState.h"

#include <string>

namespace Volt
{
	enum class RenderGraphPassFlags
	{
		None = 0,
		NeverCull = BIT(0),
		Compute = BIT(1),
		Clear = BIT(2),
		Raster = BIT(3),
		Copy = BIT(4)
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(RenderGraphPassFlags);

	class RGPass
	{ 
	public:
		struct ResourceAccess
		{
			RGResourceRef resource;
			RGResourceAccess accessType;
		};

		template<typename T>
		RGPass(const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata, RenderGraphDataAllocator* dataAllocator)
			: m_passParameters(shaderParameters, shaderParameterMetadata),
			m_dataAllocator(dataAllocator)
		{
			SetupAllocators(dataAllocator);
		}

		VT_NODISCARD VT_INLINE bool IsCulled() const { return m_isCulled; }
		VT_NODISCARD VT_INLINE RenderGraphPassFlags GetFlags() const { return m_flags; }

		/*
			Gets or creates a state of a texture or buffer.
			This will also add the necessary referecenes
		*/
		VT_NODISCARD RGTextureState& GetOrCreateTextureState(RGTextureSRVRef textureSRV);
		VT_NODISCARD RGTextureState& GetOrCreateTextureState(RGTextureUAVRef textureUAV);
		VT_NODISCARD RGTextureState& GetOrCreateTextureState(RGTextureRef texture, RGResourceAccessType accessType);
		VT_NODISCARD RGBufferState& GetOrCreateBufferState(RGBufferSRVRef bufferSRV);
		VT_NODISCARD RGBufferState& GetOrCreateBufferState(RGBufferUAVRef bufferUAV);
		VT_NODISCARD RGBufferState& GetOrCreateBufferState(RGBufferRef buffer, RGResourceAccessType accessType);
		VT_NODISCARD RGBufferState& GetOrCreateBufferState(RGUniformBufferRef buffer);

	private:
		VTRC_API void SetupAllocators(RenderGraphDataAllocator* dataAllocator);

		friend class RenderGraph;
		friend class RenderGraphPassAllocator;

		std::string m_name;
		void* m_passAllocationStartPtr;
		uint32_t passIndex = 0;
		uint32_t m_refCount = 0;
		bool m_isCulled = false;
		RenderGraphPassFlags m_flags = RenderGraphPassFlags::None;

		RenderGraphParameterStruct m_passParameters;
		RenderGraphDataAllocator* m_dataAllocator;

		RGVector<RGTextureState> m_textureStates;
		RGVector<RGBufferState> m_bufferStates;
		RGVector<RGPass*> m_passDependencies;
	};

	using RGPassRef = RGPass*;
}
