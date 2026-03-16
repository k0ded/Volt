#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Buffers/Buffer.h"
#include "RHIModule/Images/Image.h"
#include "RHIModule/Graphics/Swapchain.h"
#include "RHIModule/RHICapabilities.h"

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Containers/AtomicBitVector.h>

namespace Volt::RHI
{
	class VTRHI_API ResourceTable : public RHIInterface
	{
	public:
		inline static constexpr uint32_t MaxSize = 8192;

		virtual ~ResourceTable() = default;

		virtual void AddBuffer(RefPtr<Buffer> buffer) = 0;
		virtual void AddTexture(RefPtr<Image> texture) = 0;

		virtual void RemoveBuffer(RefPtr<Buffer> buffer) = 0;
		virtual void RemoveTexture(RefPtr<Image> texture) = 0;

		virtual uint32_t GetBufferSlotIndex(RefPtr<Buffer> buffer) = 0;
		virtual uint32_t GetTextureSlotIndex(RefPtr<Image> texture) = 0;

		virtual uint32_t GetOrAddBufferSlotIndex(RefPtr<Buffer> buffer) = 0;
		virtual uint32_t GetOrAddTextureSlotIndex(RefPtr<Image> texture) = 0;

		virtual void Update(uint32_t index) = 0;

		static RefPtr<ResourceTable> Create();

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

		template<typename ResourceType, typename ViewType>
		class Table
		{
		public:
			Table();

			void Add(RefPtr<ResourceType> resource);
			void Remove(RefPtr<ResourceType> resource);

			uint32_t GetOrAddSlotForResource(RefPtr<ResourceType> resource);
			uint32_t GetSlotForResource(RefPtr<ResourceType> resource);

			RefPtr<ResourceType> GetAtSlot(uint32_t slot);
			RefPtr<ViewType> GetViewAtSlot(uint32_t slot);

			Vector<uint32_t> GetAndClearDirtySlots(uint32_t index);

		private:
			struct Slot
			{
				uint32_t index;
				uint32_t refCount = 0;
			};

			Vector<RefPtr<ResourceType>> m_table;
			Vector<RefPtr<ViewType>> m_viewTable;
			Map<RefPtr<ResourceType>, Slot> m_resourceToIndex;
			Vector<Vector<uint32_t>> m_dirtySlots;

			ResourceIndices m_resourceIndices;
		};
	};

	template<typename ResourceType, typename ViewType>
	ResourceTable::Table<ResourceType, ViewType>::Table()
	{
		m_table.resize(ResourceTable::MaxSize);
		m_viewTable.resize(ResourceTable::MaxSize);
		m_resourceToIndex.reserve(ResourceTable::MaxSize);
		m_dirtySlots.resize(RHI::RHICapabilities::NumFramesInFlight);
	}

	template<typename ResourceType, typename ViewType>
	void ResourceTable::Table<ResourceType, ViewType>::Add(RefPtr<ResourceType> resource)
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

	template<typename ResourceType, typename ViewType>
	void ResourceTable::Table<ResourceType, ViewType>::Remove(RefPtr<ResourceType> resource)
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


	template<typename ResourceType, typename ViewType>
	uint32_t ResourceTable::Table<ResourceType, ViewType>::GetOrAddSlotForResource(RefPtr<ResourceType> resource)
	{
		auto it = m_resourceToIndex.find(resource);
		if (it == m_resourceToIndex.end())
		{
			Add(resource);
		}

		return GetSlotForResource(resource);
	}

	template<typename ResourceType, typename ViewType>
	uint32_t ResourceTable::Table<ResourceType, ViewType>::GetSlotForResource(RefPtr<ResourceType> resource)
	{
		return m_resourceToIndex.at(resource).index;
	}

	template<typename ResourceType, typename ViewType>
	Vector<uint32_t> ResourceTable::Table<ResourceType, ViewType>::GetAndClearDirtySlots(uint32_t index)
	{
		Vector<uint32_t> dirtySlots = m_dirtySlots.at(index);
		m_dirtySlots.at(index).clear();

		return dirtySlots;
	}

	template<typename ResourceType, typename ViewType>
	RefPtr<ResourceType> ResourceTable::Table<ResourceType, ViewType>::GetAtSlot(uint32_t slot)
	{
		return m_table.at(slot);
	}

	template<typename ResourceType, typename ViewType>
	RefPtr<ViewType> ResourceTable::Table<ResourceType, ViewType>::GetViewAtSlot(uint32_t slot)
	{
		if (m_viewTable[slot] != nullptr)
		{
			return m_viewTable[slot];
		}

		auto view = m_table.at(slot)->GetView();
		m_viewTable[slot] = view;

		return view;
	}
}
