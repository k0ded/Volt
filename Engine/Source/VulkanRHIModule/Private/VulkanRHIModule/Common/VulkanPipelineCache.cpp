#include "vkpch.h"

#include "VulkanRHIModule/Common/VulkanPipelineCache.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"

#include <CoreUtilities/Archive/FileArchive.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	void VulkanPipelineCache::Initialize(const std::filesystem::path& pipelineCacheFilepath)
	{
		m_cachePath = pipelineCacheFilepath;

		Vector<uint8_t> cachedData;

		FileReader fileReader{};
		if (fileReader.Open(pipelineCacheFilepath))
		{
			fileReader << cachedData;
		}

		VkPipelineCacheCreateInfo cacheInfo{};
		cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		cacheInfo.pNext = nullptr;
		cacheInfo.initialDataSize = cachedData.size();
		cacheInfo.pInitialData = cachedData.data();
		
		vkCreatePipelineCache(GraphicsContext::GetDevice()->GetHandle<VkDevice>(), &cacheInfo, nullptr, &m_pipelineCache);
	}

	VulkanPipelineCache::~VulkanPipelineCache()
	{
		VT_ENSURE_MSG(m_pipelineCache == nullptr, "The pipeline cache has not been destroyed properly!");
	}

	void VulkanPipelineCache::Shutdown()
	{
		VkDevice vkDevice = GraphicsContext::GetDevice()->GetHandle<VkDevice>();

		size_t cacheSize = 0;
		vkGetPipelineCacheData(vkDevice, m_pipelineCache, &cacheSize, nullptr);

		Vector<uint8_t> cachedData;
		cachedData.resize_uninitialized(cacheSize);

		vkGetPipelineCacheData(vkDevice, m_pipelineCache, &cacheSize, cachedData.data());

		FileWriter fileWriter{};
		if (!fileWriter.Open(m_cachePath))
		{
			return;
		}

		fileWriter << cachedData;
		fileWriter.Close();

		vkDestroyPipelineCache(vkDevice, m_pipelineCache, nullptr);
		m_pipelineCache = nullptr;
	}
}
