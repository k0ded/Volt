#pragma once

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Containers/VectorVariants.h>

// #TODO_Ivar: Switch to our own version.
#include <bitset>
#include <algorithm>

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
		IndexBuffer,
		CopyDst,
		CopySrc
	};

	class RGResourceSRV;
	class RGResourceUAV;

	class RGResource
	{
	public:
		virtual ~RGResource() = default;
		virtual RGResourceType GetResourceType() const = 0;

		// Checks if this UAV description has been produced.
		virtual bool HasProducer(RGResourceUAV* uav) const = 0;

		// Checks if this resource has been produced at all.
		virtual bool HasProducer() const = 0;

		// Adds a producer that produces the specific UAV desc
		virtual void AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav) = 0;

		// Adds a producer that produces the entire resource.
		virtual void AddProducer(Handle<RenderGraphPass> pass) = 0;

		VT_INLINE bool IsProducer(Handle<RenderGraphPass> pass) { auto it = std::find(producers.begin(), producers.end(), pass); return it != producers.end(); }
		VT_INLINE bool IsFirstProducer(Handle<RenderGraphPass> pass) { return (!producers.empty() && producers.front() == pass); }
		VT_INLINE Handle<RenderGraphPass> GetFirstProducer() const { return producers.front(); }

		VT_INLINE void AddRef() { ++m_refCount; }
		VT_INLINE void DecRef() { --m_refCount; }
		VT_INLINE uint32_t GetRefCount() const { return m_refCount; }

		// Note: We assume a maximum number of producers per resource here.
		Vector<Handle<RenderGraphPass>, InlineAllocator<32>> producers;
		Handle<RenderGraphPass> lastUser;

		bool isExternal = false;
		bool isExtracted = false;

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
