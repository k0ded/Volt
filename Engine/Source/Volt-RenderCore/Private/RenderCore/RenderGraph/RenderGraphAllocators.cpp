#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphAllocators.h"

namespace Volt
{
	RenderGraphResourceAllocator::RenderGraphResourceAllocator(RenderGraphDataAllocator* dataAllocator)
	{
		m_nodeDestructors.set_allocator({ dataAllocator });
		m_allocator.ReservePages(1);
	}

	RenderGraphResourceAllocator::~RenderGraphResourceAllocator()
	{
		VT_ASSERT(m_nodeDestructors.empty());
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

	void RenderGraphResourceAllocator::Release()
	{
		for (auto& destructor : m_nodeDestructors)
		{
			destructor.Destroy();
		}
		m_nodeDestructors.clear();
	}

	RenderGraphPassAllocator::~RenderGraphPassAllocator()
	{
		VT_ASSERT(m_passDestructors.empty());
	}

	RenderGraphPassAllocator::RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept
		: m_passExecutionFunctionAllocator(std::move(other.m_passExecutionFunctionAllocator)),
		m_passNodeAllocator(std::move(other.m_passNodeAllocator)),
		m_numPasses(std::move(other.m_numPasses)),
		m_passDestructors(std::move(other.m_passDestructors))
	{
	}

	RenderGraphPassAllocator::RenderGraphPassAllocator(RenderGraphDataAllocator* dataAllocator)
		: m_dataAllocator(dataAllocator)
	{
		m_passDestructors.set_allocator({ dataAllocator });

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

	void RenderGraphPassAllocator::Release()
	{
		for (auto& destructor : m_passDestructors)
		{
			destructor.Destroy();
		}
		m_passDestructors.clear();
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

	void RenderGraphPassAllocator::ExecutePass(RGPassRef pass, RenderContext& renderContext)
	{
		uint8_t* passAllocationPtr = reinterpret_cast<uint8_t*>(pass->m_passAllocationStartPtr);

		PassExecFunc execFunc = *(PassExecFunc*)passAllocationPtr;
		passAllocationPtr += sizeof(PassExecFunc);
		passAllocationPtr += sizeof(size_t);

		execFunc(passAllocationPtr, renderContext);
	}
}
