#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/Common/VulkanPipelineCache.h"

#include <RHIModule/Graphics/GraphicsContext.h>

struct VkInstance_T;
struct VkDebugUtilsMessengerEXT_T;
struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class PhysicalGraphicsDevice;
	class GraphicsDevice;
	class VulkanDebugLayer;
	class VulkanDescriptorHeap;
	class RayTracingTableDescriptorSetManager;
	class StaticSamplerDescriptorSetManager;

	class VulkanGraphicsContext final : public GraphicsContext
	{
	public:
		VulkanGraphicsContext(const GraphicsContextCreateInfo& createInfo);
		~VulkanGraphicsContext() override;

		VT_NODISCARD VT_INLINE VulkanDescriptorHeap& GetDescriptorHeap() const { return *m_descriptorHeap; }
		VT_NODISCARD VT_INLINE const VulkanPipelineCache& GetPipelineCache() const { return m_pipelineCache; }
		VT_NODISCARD VT_INLINE VkDescriptorSetLayout_T* GetEmptyDescriptorSetLayout() const { return m_emptyDescriptorSetLayout; }

	protected:
		RefPtr<GPUAllocator> GetDefaultAllocatorImpl() override;
		RefPtr<GPUAllocator> GetTransientAllocatorImpl() override;

		RefPtr<GraphicsDevice> GetGraphicsDevice() const override;
		RefPtr<PhysicalGraphicsDevice> GetPhysicalGraphicsDevice() const override;

		void* GetHandleImpl() const override;

	private:
		void Initialize();
		void Shutdown();
		void CreateInstance();

		void CreateEmptyDescriptorSetLayout();
		void DestroyEmptyDescriptorSetLayout();

		const Vector<const char*> GetRequiredExtensions() const;

		VkInstance_T* m_instance = nullptr;
		VkDebugUtilsMessengerEXT_T* m_debugMessenger = nullptr;
		VkDescriptorSetLayout_T* m_emptyDescriptorSetLayout = nullptr;

		RefPtr<GraphicsDevice> m_graphicsDevice;
		RefPtr<PhysicalGraphicsDevice> m_physicalDevice;

		RefPtr<GPUAllocator> m_defaultAllocator;
		RefPtr<GPUAllocator> m_transientAllocator;

		Ref<VulkanDebugLayer> m_debugLayer;
		Ref<VulkanDescriptorHeap> m_descriptorHeap;
		Ref<RayTracingTableDescriptorSetManager> m_rayTracingTableDescriptorSetManager;
		Ref<StaticSamplerDescriptorSetManager> m_staticSamplerDescriptorSetManager;

		VulkanPipelineCache m_pipelineCache;

		GraphicsContextCreateInfo m_createInfo{};
	};
}
