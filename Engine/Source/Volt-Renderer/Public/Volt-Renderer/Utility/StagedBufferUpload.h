#pragma once

#include <RenderCore/RenderGraph2/RenderGraph2.h>
#include <RenderCore/RenderGraph2/RenderContext2.h>
#include <RenderCore/RenderGraph2/RenderGraphUtils.h>

#include <atomic>

namespace Volt
{
	template<typename T>
	class StagedBufferUpload
	{
	public:
		StagedBufferUpload(const uint32_t uploadCount);

		T& AddUploadItem();
		void UploadTo(RenderGraph2& renderGraph, RGBufferRef dstBuffer);

	private:
		Vector<T> m_data;
		std::atomic<uint32_t> m_currentIndex = 0;
	};

	template<typename T>
	inline StagedBufferUpload<T>::StagedBufferUpload(const uint32_t uploadCount)
	{
		m_data.resize(uploadCount);
	}
	
	template<typename T>
	inline T& StagedBufferUpload<T>::AddUploadItem()
	{
		const uint32_t index = m_currentIndex++;
		return m_data[index];
	}

	template<typename T>
	inline void StagedBufferUpload<T>::UploadTo(RenderGraph2& renderGraph, RGBufferRef dstBuffer)
	{
		if (m_currentIndex == 0)
		{
			return;
		}

		const size_t dataSize = m_data.size() * sizeof(T);
		RGBufferRef stagingBuffer = renderGraph.CreateBuffer(RGBufferDesc::CreateStagingDesc(dataSize));
		AddMappedBufferUpload(renderGraph, stagingBuffer, m_data.data(), dataSize);
		AddCopyBufferPass(renderGraph, stagingBuffer, 0, dstBuffer, 0, dataSize);
	}
}
