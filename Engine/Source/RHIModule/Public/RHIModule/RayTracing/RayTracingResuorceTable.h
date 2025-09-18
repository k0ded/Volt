#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Buffers/StorageBuffer.h"
#include "RHIModule/Images/Image.h"
#include "RHIModule/Graphics/Swapchain.h"

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Containers/AtomicBitVector.h>

namespace Volt::RHI
{
	class VTRHI_API RayTracingResourceTable : public RHIInterface
	{
	public:
		inline static constexpr uint32_t MaxSize = 8192;

		virtual ~RayTracingResourceTable() = default;

		virtual void AddBuffer(RefPtr<StorageBuffer> buffer) = 0;
		virtual void AddTexture(RefPtr<Image> texture) = 0;

		virtual void RemoveBuffer(RefPtr<StorageBuffer> buffer) = 0;
		virtual void RemoveTexture(RefPtr<Image> texture) = 0;

		virtual uint32_t GetBufferSlotIndex(RefPtr<StorageBuffer> buffer) = 0;
		virtual uint32_t GetTextureSlotIndex(RefPtr<Image> texture) = 0;

		virtual void Update(uint32_t index) = 0;

		static RefPtr<RayTracingResourceTable> Create();

	protected:
		class VTRHI_API ResourceIndices
		{
		public:
			ResourceIndices();

			uint32_t Allocate();
			void Free(uint32_t index);

		private:
			std::atomic_uint32_t m_nextIndex;
			AtomicStack<uint32_t> m_availableIndices;
		};

		template<typename ResourceType>
		class ResourceTable
		{
		public:
			ResourceTable();

			void Add(RefPtr<ResourceType> resource);
			void Remove(RefPtr<ResourceType> resource);

			uint32_t GetSlotForResource(RefPtr<ResourceType> resource);

			RefPtr<ResourceType> GetAtSlot(uint32_t slot);
			RefPtr<ImageView> GetViewAtSlot(uint32_t slot);

			Vector<uint32_t> GetAndClearDirtySlots(uint32_t index);

		private:
			struct Slot
			{
				uint32_t index;
				uint32_t refCount = 0;
			};

			Vector<RefPtr<ResourceType>> m_table;
			Vector<RefPtr<ImageView>> m_viewTable;
			Map<RefPtr<ResourceType>, Slot> m_resourceToIndex;
			Vector<Vector<uint32_t>> m_dirtySlots;

			ResourceIndices m_resourceIndices;
		};
	};

	template<typename ResourceType>
	RayTracingResourceTable::ResourceTable<ResourceType>::ResourceTable()
	{
		m_table.resize(RayTracingResourceTable::MaxSize);
		m_resourceToIndex.reserve(RayTracingResourceTable::MaxSize);
		m_dirtySlots.resize(Swapchain::FramesInFlight);
	}

	template<typename ResourceType>
	void RayTracingResourceTable::ResourceTable<ResourceType>::Add(RefPtr<ResourceType> resource)
	{
		auto it = m_resourceToIndex.find(resource);
		if (it != m_resourceToIndex.end())
		{
			(*it).second.refCount++;
			return;
		}

		uint32_t index = m_resourceIndices.Allocate();
		m_table[index] = resource;

		m_resourceToIndex[resource] = { index, 1 };
	
		for (size_t i = 0; i < m_dirtySlots.size(); ++i)
		{
			m_dirtySlots[i].emplace_back(index);
		}
	}

	template<typename ResourceType>
	void RayTracingResourceTable::ResourceTable<ResourceType>::Remove(RefPtr<ResourceType> resource)
	{
		auto it = m_resourceToIndex.find(resource);
		if (it != m_resourceToIndex.end())
		{
			auto& slot = (*it).second;

			slot.refCount--;

			if (slot.refCount == 0)
			{
				m_table[slot.index] = nullptr;
				m_viewTable[slot.index] = nullptr;

				m_resourceIndices.Free(slot.index);
			}

			m_resourceToIndex.erase(it);
		}
	}

	template<typename ResourceType>
	uint32_t RayTracingResourceTable::ResourceTable<ResourceType>::GetSlotForResource(RefPtr<ResourceType> resource)
	{
		return m_resourceToIndex.at(resource).index;
	}

	template<typename ResourceType>
	Vector<uint32_t> RayTracingResourceTable::ResourceTable<ResourceType>::GetAndClearDirtySlots(uint32_t index)
	{
		Vector<uint32_t> dirtySlots = m_dirtySlots.at(index);
		m_dirtySlots.at(index).clear();

		return dirtySlots;
	}

	template<typename ResourceType>
	RefPtr<ResourceType> RayTracingResourceTable::ResourceTable<ResourceType>::GetAtSlot(uint32_t slot)
	{
		return m_table.at(slot);
	}

	template<typename ResourceType>
	RefPtr<ImageView> RayTracingResourceTable::ResourceTable<ResourceType>::GetViewAtSlot(uint32_t slot)
	{
		auto view = m_table.at(slot)->GetView();
		m_viewTable[slot] = view;

		return view;
	}
}
