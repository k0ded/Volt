#include "vkpch.h"

#include "VulkanRHIModule/Common/VulkanPipelineCache.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"

#include <JobSystem/IOThreads/FileIORequest.h>
#include <JobSystem/IOThreads/IOThreads.h>

#include <Volt-FileSystem/FileArchive.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	void VulkanPipelineCache::Initialize(const std::filesystem::path& pipelineCacheFilepath)
	{
		m_cachePath = pipelineCacheFilepath;

		Vector<uint8_t> cachedData;

		IORequestResult<IORequestReadFile> ioResult = IOThreads::SubmitRequest<IORequestReadFile>("Read Pipeline Cache", pipelineCacheFilepath);
		FileReader& fileReader = ioResult.GetResult();

		if (ioResult.GetResultCode() == IORequestResultCode::Success)
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

		IOThreads::SubmitRequest<IORequestWriteFile>("Write Pipeline Cache", std::move(fileWriter)),

		vkDestroyPipelineCache(vkDevice, m_pipelineCache, nullptr);
		m_pipelineCache = nullptr;
	}
}
