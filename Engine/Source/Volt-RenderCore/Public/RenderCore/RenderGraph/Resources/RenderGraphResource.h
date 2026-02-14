#pragma once

#include <RHIModule/Core/ResourceStateTracker.h>

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Containers/VectorVariants.h>

// #TODO_Ivar: Switch to our own version.
#include <bitset>
#include <algorithm>

namespace Volt
{
	class RGPass;
	class RGResourceSRV;
	class RGResourceUAV;

	enum class RGResourceType : uint8_t
	{
		Texture,
		Buffer,
		UniformBuffer
	};

	enum class RGResourceAccess : uint8_t
	{
		None,
		IndirectArg,
		VertexBuffer,
		IndexBuffer,
		CopyDst,
		CopySrc
	};

	enum class RGResourceAccessType : uint8_t
	{
		Read,
		Write
	};

	struct RGResourceAccessState
	{
		RGPass* pass = nullptr;
		uint32_t stateIndex;
		RGResourceAccessType accessType;
	};

	struct RGSubResourceState
	{
		RGSubResourceState();

		void AddState(RHI::BarrierStage stage, RHI::BarrierAccess access, RHI::ImageLayout layout);

		RHI::ResourceState state;
		RHI::ResourceState previousState;
	};

	using RGTextureSubResourceState = Vector<RGSubResourceState*>;

	class RGResource
	{
	public:
		virtual ~RGResource() = default;
		virtual RGResourceType GetResourceType() const = 0;

		VT_NODISCARD VT_INLINE uint32_t GetRefCount() const { return m_refCount; }
		VT_NODISCARD VT_INLINE bool IsExternal() const { return m_isExternal; }
		VT_NODISCARD VT_INLINE bool IsExtracted() const { return m_isExtracted; }
		VT_NODISCARD VT_INLINE bool IsProduced() const { return m_isProduced; }

	private:
		friend class RenderGraph;
		friend class RenderGraphResourceManager;
		friend class RenderGraphShaderParameterUniformBuffer;

		uint32_t m_refCount = 0;

		bool m_isExternal = false;
		bool m_isExtracted = false;
		bool m_isProduced = false;
	};

	using RGResourceRef = RGResource*;

	class RGResourceSRV
	{
	public:
		virtual ~RGResourceSRV() = default;
		virtual RGResourceRef GetResource() const = 0;
	};

	class RGResourceUAV
	{
	public:
		virtual ~RGResourceUAV() = default;
		virtual RGResourceRef GetResource() const = 0;
	};
}
