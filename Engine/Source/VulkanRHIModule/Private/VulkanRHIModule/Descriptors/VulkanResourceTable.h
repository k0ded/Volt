#pragma once

#include "VulkanRHIModule/Descriptors/VulkanDescriptorCommon.h"

#include <RHIModule/Descriptors/ResourceTable.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/AtomicBitVector.h>
#include <CoreUtilities/Allocators/Handle.h>

struct VkDescriptorSet_T;
struct VkDescriptorPool_T;

namespace Volt::RHI
{
	class VulkanResourceTable : public ResourceTable
	{
	public:
		VulkanResourceTable();
		~VulkanResourceTable() override;

		void AddBuffer(IntRef<Buffer> buffer) override;
		void AddTexture(IntRef<Image> texture) override;

		void RemoveBuffer(IntRef<Buffer> buffer) override;
		void RemoveTexture(IntRef<Image> texture) override;

		uint32_t GetBufferSlotIndex(IntRef<Buffer> buffer) override;
		uint32_t GetTextureSlotIndex(IntRef<Image> texture) override;

		uint32_t GetOrAddBufferSlotIndex(IntRef<Buffer> buffer) override;
		uint32_t GetOrAddTextureSlotIndex(IntRef<Image> texture) override;

		void Update(uint32_t index) override;

		uint8_t* GetHeapPointer();
		uint64_t GetBaseOffset() const;
		uint64_t GetDeviceAddress() const;

	private:
		void* GetHandleImpl() const override;

		void Initialize();
		void Release();

		ResourceIndices m_textureResourceIndices;
		ResourceIndices m_bufferResourceIndices;

		ResourceTable::Table<Buffer, BufferView> m_bufferTable;
		ResourceTable::Table<Image, ImageView> m_textureTable;
	
		uint32_t m_lastUpdateIndex = 0;
		uint32_t m_currentBufferIndex = 0;

		Handle<Allocation> m_descriptorHeapAllocation;
		uint64_t m_descriptorSetLayoutSize = 0;
		uint8_t* m_mappedPtr = nullptr;
	};
}
