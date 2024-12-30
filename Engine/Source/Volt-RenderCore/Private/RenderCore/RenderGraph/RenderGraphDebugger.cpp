#include "rcpch.h"
#include "RenderCore/RenderGraph/RenderGraphDebugger.h"
#include "RenderCore/RenderGraph/RenderGraph.h"

namespace Volt
{
	static ConsoleVariable<int32_t> s_enableRenderGraphDebugger("r.RenderGraph.EnableDebug", 0, "Whether or not RenderGraph debugging is enabled or not.");

	void RenderGraphDebugger::ProcessRenderGraph(RenderGraph& renderGraph)
	{
		m_extractedImages.clear();
		m_extractedBuffers.clear();

		if (!s_enableRenderGraphDebugger.GetValue())
		{
			return;
		}

		m_renderGraphFence = renderGraph.m_executionFence;

		// First we need to find how many of each resource we have
		uint32_t imageCount = 0;
		uint32_t bufferCount = 0;

		for (const auto& resourceNode : renderGraph.m_resourceNodes)
		{
			if (resourceNode->GetResourceType() == ResourceType::Image2D)
			{
				imageCount++;
			}
			else if (resourceNode->GetResourceType() == ResourceType::Image3D)
			{
				imageCount++;
			}
			else if (resourceNode->GetResourceType() == ResourceType::Buffer)
			{
				bufferCount++;
			}
		}

		m_extractedImages.reserve(imageCount);
		m_extractedBuffers.reserve(bufferCount);

		for (const auto& resourceNode : renderGraph.m_resourceNodes)
		{
			if (resourceNode->GetResourceType() == ResourceType::Image2D)
			{
				renderGraph.EnqueueImageExtraction(*reinterpret_cast<RenderGraphImageHandle*>(&resourceNode->handle), m_extractedImages.emplace_back());
			}
			else if (resourceNode->GetResourceType() == ResourceType::Image3D)
			{
				renderGraph.EnqueueImageExtraction(*reinterpret_cast<RenderGraphImageHandle*>(&resourceNode->handle), m_extractedImages.emplace_back());
			}
			else if (resourceNode->GetResourceType() == ResourceType::Buffer)
			{
				renderGraph.EnqueueBufferExtraction(*reinterpret_cast<RenderGraphBufferHandle*>(&resourceNode->handle), m_extractedBuffers.emplace_back());
			}
		}
	}

	void RenderGraphDebugger::WaitForFinishedExecution() const
	{
		if (m_renderGraphFence)
		{
			m_renderGraphFence->WaitUntilSignaled();
		}
	}
}
