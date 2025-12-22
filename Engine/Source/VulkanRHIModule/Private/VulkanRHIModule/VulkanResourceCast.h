#pragma once

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"

#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Images/VulkanImage.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"
#include "VulkanRHIModule/Images/VulkanSamplerState.h"

#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanComputePipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"

#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"
#include "VulkanRHIModule/RayTracing/VulkanRayTracingResourceTable.h"
#include "VulkanRHIModule/RayTracing/VulkanAccelerationStructure.h"

#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Synchronization/VulkanFence.h"

namespace Volt::RHI
{
	template<class T>
	struct VulkanTypeTraits
	{};

	template<>
	struct VulkanTypeTraits<BufferView>
	{
		using ConcreteType = VulkanBufferView;
	};

	template<>
	struct VulkanTypeTraits<CommandBuffer>
	{
		using ConcreteType = VulkanCommandBuffer;
	};

	template<>
	struct VulkanTypeTraits<StorageBuffer>
	{
		using ConcreteType = VulkanStorageBuffer;
	};

	template<>
	struct VulkanTypeTraits<UniformBuffer>
	{
		using ConcreteType = VulkanUniformBuffer;
	};

	template<>
	struct VulkanTypeTraits<DeviceQueue>
	{
		using ConcreteType = VulkanDeviceQueue;
	};

	template<>
	struct VulkanTypeTraits<GraphicsContext>
	{
		using ConcreteType = VulkanGraphicsContext;
	};

	template<>
	struct VulkanTypeTraits<GraphicsDevice>
	{
		using ConcreteType = VulkanGraphicsDevice;
	};

	template<>
	struct VulkanTypeTraits<PhysicalGraphicsDevice>
	{
		using ConcreteType = VulkanPhysicalGraphicsDevice;
	};

	template<>
	struct VulkanTypeTraits<Swapchain>
	{
		using ConcreteType = VulkanSwapchain;
	};

	template<>
	struct VulkanTypeTraits<Image>
	{
		using ConcreteType = VulkanImage;
	};

	template<>
	struct VulkanTypeTraits<ImageView>
	{
		using ConcreteType = VulkanImageView;
	};

	template<>
	struct VulkanTypeTraits<SamplerState>
	{
		using ConcreteType = VulkanSamplerState;
	};

	template<>
	struct VulkanTypeTraits<RenderPipeline>
	{
		using ConcreteType = VulkanRenderPipeline;
	};

	template<>
	struct VulkanTypeTraits<ComputePipeline>
	{
		using ConcreteType = VulkanComputePipeline;
	};

	template<>
	struct VulkanTypeTraits<RayTracingPipeline>
	{
		using ConcreteType = VulkanRayTracingPipeline;
	};

	template<>
	struct VulkanTypeTraits<ShaderBindingTable>
	{
		using ConcreteType = VulkanShaderBindingTable;
	};

	template<>
	struct VulkanTypeTraits<RayTracingResourceTable>
	{
		using ConcreteType = VulkanRayTracingResourceTable;
	};

	template<>
	struct VulkanTypeTraits<AccelerationStructure>
	{
		using ConcreteType = VulkanAccelerationStructure;
	};

	template<>
	struct VulkanTypeTraits<Shader>
	{
		using ConcreteType = VulkanShader;
	};

	template<>
	struct VulkanTypeTraits<ShaderCompiler>
	{
		using ConcreteType = VulkanShaderCompiler;
	};

	template<>
	struct VulkanTypeTraits<Fence>
	{
		using ConcreteType = VulkanFence;
	};

	template<typename RHIType>
	VT_INLINE static typename VulkanTypeTraits<RHIType>::ConcreteType* ResourceCast(RHIType* resource)
	{
		return static_cast<typename VulkanTypeTraits<RHIType>::ConcreteType*>(resource);
	}

	template<typename RHIType>
	VT_INLINE static const typename VulkanTypeTraits<RHIType>::ConcreteType* ResourceCast(const RHIType* resource)
	{
		return static_cast<const typename VulkanTypeTraits<RHIType>::ConcreteType*>(resource);
	}

	template<typename RHIType>
	VT_INLINE static RefPtr<typename VulkanTypeTraits<RHIType>::ConcreteType> ResourceCast(const RefPtr<RHIType>& resource)
	{
		using ConcreteType = typename VulkanTypeTraits<RHIType>::ConcreteType;
		return  RefPtr<ConcreteType>::Attach(static_cast<ConcreteType*>(resource.GetRaw()));
	}

	template<typename RHIType>
	VT_INLINE static const typename VulkanTypeTraits<RHIType>::ConcreteType* ResourceCastGetRaw(const RefPtr<RHIType>& resource)
	{
		return static_cast<const typename VulkanTypeTraits<RHIType>::ConcreteType*>(resource.GetRaw());
	}

	template<typename RHIType, typename ImplType>
	VT_INLINE static const ImplType* ResourceCastGetRawUnsafe(const RefPtr<RHIType>& resource)
	{
		return static_cast<const typename VulkanTypeTraits<RHIType>::ConcreteType*>(resource.GetRaw());
	}
}
