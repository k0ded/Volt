#include "rcpch.h"

#include "RenderCore/RenderGraph2/RenderGraphAllocators2.h"

namespace Volt
{
	RenderGraphResourceAllocator2::~RenderGraphResourceAllocator2()
	{
		for (auto& destructor : m_nodeDestructors)
		{
			destructor.Destroy();
		}
	}

	RenderGraphResourceAllocator2::RenderGraphResourceAllocator2(RenderGraphResourceAllocator2&& other) noexcept
		: m_allocator(std::move(other.m_allocator)),
		m_nodeDestructors(std::move(other.m_nodeDestructors))
	{
	}
	
	RenderGraphResourceAllocator2& RenderGraphResourceAllocator2::operator=(RenderGraphResourceAllocator2&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_nodeDestructors = std::move(other.m_nodeDestructors);

		return *this;
	}

	RenderGraphPassAllocator2::~RenderGraphPassAllocator2()
	{
		for (auto& destructor : m_passDestructors)
		{
			destructor.Destroy();
		}
	}

	RenderGraphPassAllocator2::RenderGraphPassAllocator2(RenderGraphPassAllocator2&& other) noexcept
		: m_numPasses(std::move(other.m_numPasses)),
		m_passExecutionFunctionAllocator(std::move(other.m_passExecutionFunctionAllocator)),
		m_passNodeAllocator(std::move(other.m_passNodeAllocator)),
		m_passDestructors(std::move(other.m_passDestructors))
	{
	}

	RenderGraphPassAllocator2& RenderGraphPassAllocator2::operator=(RenderGraphPassAllocator2&& other) noexcept
	{
		m_numPasses = std::move(other.m_numPasses);
		m_passExecutionFunctionAllocator = std::move(other.m_passExecutionFunctionAllocator);
		m_passNodeAllocator = std::move(other.m_passNodeAllocator);
		m_passDestructors = std::move(other.m_passDestructors);

		return *this;
	}

	RenderGraphPassAllocator2::PassAllocation RenderGraphPassAllocator2::AllocatePass(PassExecFunc execWrapperFunc, size_t execFuncSize)
	{
		const size_t totalAllocationSize = sizeof(PassExecFunc) + execFuncSize + sizeof(execFuncSize);

		uint8_t* allocation = reinterpret_cast<uint8_t*>(m_passExecutionFunctionAllocator.Allocate(totalAllocationSize));

		PassAllocation result{};
		result.passAllocationStartPtr = allocation;

		*(PassExecFunc*)allocation = execWrapperFunc;
		allocation += sizeof(PassExecFunc);

		*(size_t*)allocation = execFuncSize;
		allocation += sizeof(size_t);

		result.executionFunctionPtr = allocation;

		return result;
	}

	void RenderGraphPassAllocator2::ExecutePass(Handle<RenderGraphPass> pass, RenderContext2& renderContext)
	{
		uint8_t* passAllocationPtr = reinterpret_cast<uint8_t*>(pass->passAllocationStartPtr);

		PassExecFunc execFunc = *(PassExecFunc*)passAllocationPtr;
		passAllocationPtr += sizeof(PassExecFunc);
		passAllocationPtr += sizeof(size_t);

		execFunc(passAllocationPtr, renderContext);
	}
}
