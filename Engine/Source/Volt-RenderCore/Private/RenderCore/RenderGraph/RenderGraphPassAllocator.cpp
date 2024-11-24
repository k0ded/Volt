#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphPassAllocator.h"

namespace Volt
{
	RenderGraphPassAllocator::RenderGraphPassAllocator(const RenderGraphPassAllocator& other) noexcept
		: m_numPasses(other.m_numPasses),
		m_passExecutionFunctionAllocator(other.m_passExecutionFunctionAllocator),
		m_passNodeAllocator(other.m_passNodeAllocator)
	{
	}
	
	RenderGraphPassAllocator::RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept
		: m_numPasses(std::move(other.m_numPasses)),
		m_passExecutionFunctionAllocator(std::move(other.m_passExecutionFunctionAllocator)),
		m_passNodeAllocator(std::move(other.m_passNodeAllocator))
	{
	}

	RenderGraphPassAllocator& RenderGraphPassAllocator::operator=(const RenderGraphPassAllocator& other) noexcept
	{
		m_numPasses = other.m_numPasses;
		m_passExecutionFunctionAllocator = other.m_passExecutionFunctionAllocator;
		m_passNodeAllocator = other.m_passNodeAllocator;

		return *this;
	}
	
	RenderGraphPassAllocator& RenderGraphPassAllocator::operator=(RenderGraphPassAllocator&& other) noexcept
	{
		m_numPasses = std::move(other.m_numPasses);
		m_passExecutionFunctionAllocator = std::move(other.m_passExecutionFunctionAllocator);
		m_passNodeAllocator = std::move(other.m_passNodeAllocator);

		return *this;
	}
	
	void RenderGraphPassAllocator::ExecutePass(Handle<RenderGraphPassNodeBase> passNode, RenderContext& renderContext)
	{
		uint8_t* passAllocationPtr = reinterpret_cast<uint8_t*>(passNode->passAllocationStartPtr);

		PassExecFunc execFunc = *(PassExecFunc*)passAllocationPtr;
		passAllocationPtr += sizeof(PassExecFunc);

		const size_t size = *(size_t*)passAllocationPtr;
		VT_UNUSED(size);

		passAllocationPtr += sizeof(size_t);

		execFunc(passAllocationPtr, passNode->GetDataPointer(), renderContext);
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
}
