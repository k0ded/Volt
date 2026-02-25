#pragma once

#include "RenderCore/RenderGraph/RenderGraphContainerAllocator.h"

#include <RHIModule/Core/ResourceStateTracker.h>

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Containers/VectorVariants.h>

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
		CopySrc,
		Upload
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

	using RGTextureSubResourceState = RGVector<RGSubResourceState*>;

	class RGResource
	{
	public:
		RGResource(uint32_t resourceId);
		virtual ~RGResource() = default;
		virtual RGResourceType GetResourceType() const = 0;

		VT_NODISCARD VT_INLINE uint32_t GetRefCount() const { return m_refCount; }
		VT_NODISCARD VT_INLINE bool IsExternal() const { return m_isExternal; }
		VT_NODISCARD VT_INLINE bool IsExtracted() const { return m_isExtracted; }
		VT_NODISCARD VT_INLINE bool IsProduced() const { return m_isProduced; }
		VT_NODISCARD VT_INLINE uint32_t GetResourceID() const { return m_resourceId; }

		RGPass* firstPassAccessor = nullptr;

	private:
		friend class RenderGraph;
		friend class RenderGraphDebugger;
		friend class RenderGraphResourceManager;
		friend class RenderGraphShaderParameterUniformBuffer;

		uint32_t m_refCount = 0;
		uint32_t m_resourceId;

		bool m_isExternal : 1 = false;
		bool m_isExtracted : 1 = false;
		bool m_isProduced : 1 = false;
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

	template<typename T>
		requires(std::is_base_of_v<RGResource, T>)
	inline T* ResourceCast(RGResourceRef resource)
	{
		return reinterpret_cast<T*>(resource);
	}

	template<typename T>
		requires(std::is_base_of_v<RGResourceSRV, T>)
	inline T* ResourceSRVCast(RGResourceSRV* resource)
	{
		return reinterpret_cast<T*>(resource);
	}

	template<typename T>
		requires(std::is_base_of_v<RGResourceUAV, T>)
	inline T* ResourceUAVCast(RGResourceUAV* resource)
	{
		return reinterpret_cast<T*>(resource);
	}
}
