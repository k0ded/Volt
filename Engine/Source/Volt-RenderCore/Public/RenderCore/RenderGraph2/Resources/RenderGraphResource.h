#pragma once

#include <CoreUtilities/Allocators/Handle.h>

namespace Volt
{
	class RenderGraphPass;

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
		IndexBuffer
	};

	class RGResource
	{
	public:
		virtual ~RGResource() = default;
		virtual RGResourceType GetResourceType() const = 0;

		VT_INLINE void AddRef() { ++m_refCount; }
		VT_INLINE void DecRef() { --m_refCount; }
		VT_INLINE uint32_t GetRefCount() const { return m_refCount; }

		bool isProduced = false;
		bool isExternal = false;

		Handle<RenderGraphPass> producer;
		Handle<RenderGraphPass> lastUser;

	private:
		uint32_t m_refCount = 0;
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
