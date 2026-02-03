#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphAllocators.h"

namespace Volt
{
	RenderGraphResourceAllocator::RenderGraphResourceAllocator()
	{
		m_allocator.ReservePages(1);
	}

	RenderGraphResourceAllocator::~RenderGraphResourceAllocator()
	{
		for (auto& destructor : m_nodeDestructors)
		{
			destructor.Destroy();
		}
	}

	RenderGraphResourceAllocator::RenderGraphResourceAllocator(RenderGraphResourceAllocator&& other) noexcept
		: m_allocator(std::move(other.m_allocator)),
		m_nodeDestructors(std::move(other.m_nodeDestructors))
	{
	}
	
	RenderGraphResourceAllocator& RenderGraphResourceAllocator::operator=(RenderGraphResourceAllocator&& other) noexcept
	{
		m_allocator = std::move(other.m_allocator);
		m_nodeDestructors = std::move(other.m_nodeDestructors);

		return *this;
	}

	RenderGraphPassAllocator::~RenderGraphPassAllocator()
	{
		for (auto& destructor : m_passDestructors)
		{
			destructor.Destroy();
		}
	}

	RenderGraphPassAllocator::RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept
		: m_numPasses(std::move(other.m_numPasses)),
		m_passExecutionFunctionAllocator(std::move(other.m_passExecutionFunctionAllocator)),
		m_passNodeAllocator(std::move(other.m_passNodeAllocator)),
		m_passDestructors(std::move(other.m_passDestructors))
	{
	}

	RenderGraphPassAllocator::RenderGraphPassAllocator()
	{
		m_passExecutionFunctionAllocator.ReservePages(1);
		m_passNodeAllocator.ReservePages(1);
	}

	RenderGraphPassAllocator& RenderGraphPassAllocator::operator=(RenderGraphPassAllocator&& other) noexcept
	{
		m_numPasses = std::move(other.m_numPasses);
		m_passExecutionFunctionAllocator = std::move(other.m_passExecutionFunctionAllocator);
		m_passNodeAllocator = std::move(other.m_passNodeAllocator);
		m_passDestructors = std::move(other.m_passDestructors);

		return *this;
	}

	RenderGraphPassAllocator::PassAllocation RenderGraphPassAllocator::AllocatePass(PassExecFunc execWrapperFunc, size_t execFuncSize)
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

	void RenderGraphPassAllocator::ExecutePass(RenderGraphPassRef pass, RenderContext& renderContext)
	{
		uint8_t* passAllocationPtr = reinterpret_cast<uint8_t*>(pass->passAllocationStartPtr);

		PassExecFunc execFunc = *(PassExecFunc*)passAllocationPtr;
		passAllocationPtr += sizeof(PassExecFunc);
		passAllocationPtr += sizeof(size_t);

		execFunc(passAllocationPtr, renderContext);
	}
}
