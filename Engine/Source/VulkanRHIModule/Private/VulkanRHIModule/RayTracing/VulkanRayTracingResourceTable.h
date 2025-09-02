#pragma once

#include <VulkanRHIModule/Descriptors/VulkanDescriptorCommon.h>

#include <RHIModule/RayTracing/RayTracingResuorceTable.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/AtomicBitVector.h>

struct VkDescriptorSet_T;
struct VkDescriptorPool_T;

namespace Volt::RHI
{
	class VulkanRayTracingResourceTable : public RayTracingResourceTable
	{
	public:
		VulkanRayTracingResourceTable();
		~VulkanRayTracingResourceTable() override;

		void AddBuffer(RefPtr<StorageBuffer> buffer) override;
		void AddTexture(RefPtr<Image> texture) override;

		void RemoveBuffer(RefPtr<StorageBuffer> buffer) override;
		void RemoveTexture(RefPtr<Image> texture) override;

		uint32_t GetBufferSlotIndex(RefPtr<StorageBuffer> buffer) override;
		uint32_t GetTextureSlotIndex(RefPtr<Image> texture) override;

		void Update(uint32_t index) override;
		VkDescriptorSet_T* GetDescriptorSet() const;

	private:
		void* GetHandleImpl() const override;

		void CreateDescriptorSets();

		ResourceIndices m_textureResourceIndices;
		ResourceIndices m_bufferResourceIndices;

		VkDescriptorPool_T* m_descriptorPool;
		Vector<VkDescriptorSet_T*> m_descriptorSets;

		ResourceTable<StorageBuffer> m_bufferTable;
		ResourceTable<Image> m_textureTable;
	
		Vector<DescriptorWrite> m_writeDescriptors;
		Vector<DescriptorBufferInfo> m_bufferDescriptorInfo;
		Vector<DescriptorImageInfo> m_imageDescriptorInfo;
	
		uint32_t m_lastUpdateIndex = 0;
	};
}
