#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Core/RHIInterface.h"

#include "RHIModule/Memory/GPUAllocator.h"
#include "RHIModule/Graphics/GraphicsDevice.h"
#include "RHIModule/Graphics/PhysicalGraphicsDevice.h"

namespace Volt::RHI
{
	class GraphicsDevice;
	class PhysicalGraphicsDevice;

	class VTRHI_API GraphicsContext : public RHIInterface
	{
	public:
		GraphicsContext();
		virtual ~GraphicsContext();

		VT_NODISCARD VT_INLINE static GraphicsContext& Get() { return *s_context; }
		VT_NODISCARD VT_INLINE static IntRef<GraphicsDevice> GetDevice() { return s_context->GetGraphicsDevice(); };
		VT_NODISCARD VT_INLINE static IntRef<PhysicalGraphicsDevice> GetPhysicalDevice() { return s_context->GetPhysicalGraphicsDevice(); };
		VT_NODISCARD VT_INLINE static IntRef<GPUAllocator> GetDefaultAllocator() { return s_context->GetDefaultAllocatorImpl(); };
		VT_NODISCARD VT_INLINE static GraphicsAPI GetAPI() { return s_graphicsAPI; }

		static IntRef<GraphicsContext> Create(const GraphicsContextCreateInfo& createInfo);

	protected:
		virtual IntRef<GPUAllocator> GetDefaultAllocatorImpl() = 0;

		virtual IntRef<GraphicsDevice> GetGraphicsDevice() const = 0;
		virtual IntRef<PhysicalGraphicsDevice> GetPhysicalGraphicsDevice() const = 0;

	private:
		inline static GraphicsContext* s_context;
		inline static GraphicsAPI s_graphicsAPI;
	};
}
