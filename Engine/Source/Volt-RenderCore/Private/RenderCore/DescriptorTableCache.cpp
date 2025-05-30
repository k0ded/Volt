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

		m_activeDescriptorTables.resize(RHI::Swapchain::FramesInFlight);
	}

	DescriptorTableCache::~DescriptorTableCache()
	{
		s_instance = nullptr;
	}

	RefPtr<RHI::DescriptorTable> DescriptorTableCache::GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::ComputePipeline> pipeline)
	{
		RefPtr<RHI::DescriptorTable> descriptorTable;

		if (m_descriptorTableCache.contains(pipeline->GetHash()))
		{
			auto& data = m_descriptorTableCache.at(pipeline->GetHash());

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
			descriptorTable = RHI::DescriptorTable::Create2(createInfo);
		}

		const uint32_t cacheIndex = m_frameIndex % RHI::Swapchain::FramesInFlight;
		m_activeDescriptorTables.at(cacheIndex).emplace_back(descriptorTable, pipeline->GetHash());

		return descriptorTable;
	}

	RefPtr<RHI::DescriptorTable> DescriptorTableCache::GetOrCreateDescriptorTableForPipeline(RefPtr<RHI::RenderPipeline> pipeline)
	{
		RefPtr<RHI::DescriptorTable> descriptorTable;

		if (m_descriptorTableCache.contains(pipeline->GetHash()))
		{
			auto& data = m_descriptorTableCache.at(pipeline->GetHash());

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
			descriptorTable = RHI::DescriptorTable::Create2(createInfo);
		}

		const uint32_t cacheIndex = m_frameIndex % RHI::Swapchain::FramesInFlight;
		m_activeDescriptorTables.at(cacheIndex).emplace_back(descriptorTable, pipeline->GetHash());
		
		return descriptorTable;
	}

	void DescriptorTableCache::Update()
	{
		const uint32_t cacheIndex = m_frameIndex % RHI::Swapchain::FramesInFlight;

		for (const auto& activeTable : m_activeDescriptorTables.at(cacheIndex))
		{
			if (!m_descriptorTableCache.contains(activeTable.pipelineHash))
			{
				m_descriptorTableCache[activeTable.pipelineHash].mutex = CreateRef<std::mutex>();
			}

			auto& data = m_descriptorTableCache[activeTable.pipelineHash];
			
			std::scoped_lock lock{ *data.mutex };
			data.descriptorTables.emplace_back(activeTable.descriptorTable);
		}

		m_activeDescriptorTables.at(cacheIndex).clear();

		m_frameIndex++;
	}
}
