#pragma once

#include "VulkanRHIModule/Core.h"
#include "VulkanRHIModule/Common/VulkanPipelineCache.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <CoreUtilities/Pointers/Ref.h>

struct VkInstance_T;
struct VkDebugUtilsMessengerEXT_T;
struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class PhysicalGraphicsDevice;
	class GraphicsDevice;
	class VulkanDebugLayer;
	class VulkanDescriptorHeap;
	class ResourceTableDescriptorSetManager;
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
		IntRef<GPUAllocator> GetDefaultAllocatorImpl() override;

		IntRef<GraphicsDevice> GetGraphicsDevice() const override;
		IntRef<PhysicalGraphicsDevice> GetPhysicalGraphicsDevice() const override;

		void* GetHandleImpl() const override;

	private:
		void Initialize();
		void Shutdown();
		void CreateInstance();

		void CreateEmptyDescriptorSetLayout();
		void DestroyEmptyDescriptorSetLayout();

		const Vector<const char*> GetRequiredExtensions() const;

		VkInstance_T* m_instance = nullptr;
		VkDescriptorSetLayout_T* m_emptyDescriptorSetLayout = nullptr;

		IntRef<GraphicsDevice> m_graphicsDevice;
		IntRef<PhysicalGraphicsDevice> m_physicalDevice;

		IntRef<GPUAllocator> m_defaultAllocator;
		IntRef<GPUAllocator> m_transientAllocator;

		Ref<VulkanDebugLayer> m_debugLayer;
		Ref<VulkanDescriptorHeap> m_descriptorHeap;
		Ref<ResourceTableDescriptorSetManager> m_resourceTableDescriptorSetManager;
		Ref<StaticSamplerDescriptorSetManager> m_staticSamplerDescriptorSetManager;

		VulkanPipelineCache m_pipelineCache;

		GraphicsContextCreateInfo m_createInfo{};
	};
}
