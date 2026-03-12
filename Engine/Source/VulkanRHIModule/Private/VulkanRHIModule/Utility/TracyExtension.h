#pragma once

#include <CoreUtilities/Containers/Vector.h>

#include <tracy/TracyVulkan.hpp>

#include <vulkan/vulkan.h>

namespace Volt::RHI::Tracy
{
	thread_local Vector<bool> g_isScopeActive;

	VT_INLINE void BeginProfilingScope(tracy::VkCtx* ctx, uint32_t line, const char* source, size_t sourceSz, const char* function, size_t functionSz, const char* name, size_t nameSz, VkCommandBuffer cmdbuf)
	{
		g_isScopeActive.emplace_back() = tracy::GetProfiler().IsConnected();
		
		if (!g_isScopeActive.back())
		{
			return;
		}

		const auto queryId = ctx->NextQueryId();
		CONTEXT_VK_FUNCTION_WRAPPER(vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ctx->m_query, queryId));

		const auto srcloc = tracy::Profiler::AllocSourceLocation(line, source, sourceSz, function, functionSz, name, nameSz);
		auto item = tracy::Profiler::QueueSerial();
		tracy::MemWrite(&item->hdr.type, tracy::QueueType::GpuZoneBeginAllocSrcLocSerial);
		tracy::MemWrite(&item->gpuZoneBegin.cpuTime, tracy::Profiler::GetTime());
		tracy::MemWrite(&item->gpuZoneBegin.srcloc, srcloc);
		tracy::MemWrite(&item->gpuZoneBegin.thread, tracy::GetThreadHandle());
		tracy::MemWrite(&item->gpuZoneBegin.queryId, uint16_t(queryId));
		tracy::MemWrite(&item->gpuZoneBegin.context, ctx->GetId());
		tracy::Profiler::QueueSerialFinish();
	}

	VT_INLINE void EndProfilingScope(tracy::VkCtx* ctx, VkCommandBuffer cmdbuf)
	{
		const bool active = g_isScopeActive.back();
		g_isScopeActive.pop_back();
		
		if (!active)
		{
			return;
		}

		const auto queryId = ctx->NextQueryId();
		CONTEXT_VK_FUNCTION_WRAPPER(vkCmdWriteTimestamp(cmdbuf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ctx->m_query, queryId));

		auto item = tracy::Profiler::QueueSerial();
		tracy::MemWrite(&item->hdr.type, tracy::QueueType::GpuZoneEndSerial);
		tracy::MemWrite(&item->gpuZoneEnd.cpuTime, tracy::Profiler::GetTime());
		tracy::MemWrite(&item->gpuZoneEnd.thread, tracy::GetThreadHandle());
		tracy::MemWrite(&item->gpuZoneEnd.queryId, uint16_t(queryId));
		tracy::MemWrite(&item->gpuZoneEnd.context, ctx->GetId());
		tracy::Profiler::QueueSerialFinish();
	}
}
