#include "rcpch.h"
#include "RenderCore/DescriptorTableCache.h"

#include <RHIModule/Graphics/Swapchain.h>
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

namespace Volt
{
	DescriptorTableCache::DescriptorTableCache()
	{
		VT_ENSURE(s_instance == nullptr);
		s_instance = this;
	}

	DescriptorTableCache::~DescriptorTableCache()
	{
		s_instance = nullptr;
	}

	RefPtr<RHI::DescriptorTable> DescriptorTableCache::GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::ComputePipeline> pipeline)
	{
		RefPtr<RHI::DescriptorTable> descriptorTable;

		const size_t pipelineHash = pipeline->GetHash();

		if (m_descriptorTableCache.contains(pipelineHash))
		{
			auto& data = m_descriptorTableCache.at(pipelineHash);

			std::scoped_lock lock{ *data.mutex };
			if (!data.descriptorTables.empty())
			{
				descriptorTable = data.descriptorTables.back();
				data.descriptorTables.pop_back();
			}
		}

		if (!descriptorTable)
		{
			RHI::DescriptorTableCreateInfo createInfo{};
			createInfo.computePipeline = pipeline;
			descriptorTable = RHI::DescriptorTable::Create(createInfo);
		}

		m_activeDescriptorTableCache.AddDescriptorTable(descriptorTable, pipelineHash);

		return descriptorTable;
	}

	RefPtr<RHI::DescriptorTable> DescriptorTableCache::GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::RenderPipeline> pipeline)
	{
		RefPtr<RHI::DescriptorTable> descriptorTable;

		const size_t pipelineHash = pipeline->GetHash();

		if (m_descriptorTableCache.contains(pipelineHash))
		{
			auto& data = m_descriptorTableCache.at(pipelineHash);

			std::scoped_lock lock{ *data.mutex };
			if (!data.descriptorTables.empty())
			{
				descriptorTable = data.descriptorTables.back();
				data.descriptorTables.pop_back();
			}
		}

		if (!descriptorTable)
		{
			RHI::DescriptorTableCreateInfo createInfo{};
			createInfo.renderPipeline = pipeline;
			descriptorTable = RHI::DescriptorTable::Create(createInfo);
		}

		m_activeDescriptorTableCache.AddDescriptorTable(descriptorTable, pipelineHash);
		
		return descriptorTable;
	}

	void DescriptorTableCache::FlushDescriptorTableCacheForPipeline(size_t pipelineHash)
	{
		if (m_descriptorTableCache.contains(pipelineHash))
		{
			m_descriptorTableCache.erase(pipelineHash);
		}

		m_activeDescriptorTableCache.FlushDescriptorTableCacheForPipeline(pipelineHash);
	}

	void DescriptorTableCache::Update()
	{
		Vector<ActiveDescriptorTableCache::ActiveDescriptorTable> inactiveDescriptorTables = m_activeDescriptorTableCache.UpdateAndGetInactiveDescriptorTables();

		for (const auto& inactiveTable : inactiveDescriptorTables)
		{
			if (!m_descriptorTableCache.contains(inactiveTable.pipelineHash))
			{
				m_descriptorTableCache[inactiveTable.pipelineHash].mutex = CreateRef<std::mutex>();
			}

			auto& data = m_descriptorTableCache[inactiveTable.pipelineHash];

			VT_ENSURE(inactiveTable.pipelineHash == inactiveTable.descriptorTable->GetHash());

			std::scoped_lock lock{ *data.mutex };
			data.descriptorTables.emplace_back(inactiveTable.descriptorTable);
		}
	}

	void ActiveDescriptorTableCache::AddDescriptorTable(RefPtr<RHI::DescriptorTable> descriptorTable, size_t pipelineHash)
	{
		std::scoped_lock lock{ m_mutex };
		auto& newActive = m_activeDescriptorTables.emplace_back();
		newActive.activeDescriptorTable.descriptorTable = descriptorTable;
		newActive.activeDescriptorTable.pipelineHash = pipelineHash;
		newActive.framesAlive = 0;
	}

	Vector<ActiveDescriptorTableCache::ActiveDescriptorTable> ActiveDescriptorTableCache::UpdateAndGetInactiveDescriptorTables()
	{
		Vector<ActiveDescriptorTableCache::ActiveDescriptorTable> result{};

		constexpr size_t FRAMES_ALIVE = 3;

		{
			std::scoped_lock lock{ m_mutex };
			for (int32_t i = static_cast<int32_t>(m_activeDescriptorTables.size()) - 1; i >= 0; --i)
			{
				auto& activeDescriptorTable = m_activeDescriptorTables.at(i);
				if (activeDescriptorTable.framesAlive >= FRAMES_ALIVE)
				{
					result.emplace_back(activeDescriptorTable.activeDescriptorTable);
					m_activeDescriptorTables.erase_unsorted(m_activeDescriptorTables.begin() + i);
				}
				else
				{
					activeDescriptorTable.framesAlive++;
				}
			}
		}

		return result;
	}

	void ActiveDescriptorTableCache::FlushDescriptorTableCacheForPipeline(size_t pipelineHash)
	{
		std::scoped_lock lock{ m_mutex };
		for (int32_t i = static_cast<int32_t>(m_activeDescriptorTables.size()) - 1; i >= 0; --i)
		{
			if (m_activeDescriptorTables.at(i).activeDescriptorTable.pipelineHash == pipelineHash)
			{
				m_activeDescriptorTables.erase_unsorted(m_activeDescriptorTables.begin() + i);
			}
		}
	}
}
