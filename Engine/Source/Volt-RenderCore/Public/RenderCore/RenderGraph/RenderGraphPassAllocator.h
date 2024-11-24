#pragma once

#include "RenderCore/RenderGraph/RenderGraphPass.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{
	class VTRC_API RenderGraphPassAllocator
	{
	public:
		RenderGraphPassAllocator() = default;
		~RenderGraphPassAllocator() = default;

		RenderGraphPassAllocator(const RenderGraphPassAllocator& other) noexcept;
		RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept;
		RenderGraphPassAllocator& operator=(const RenderGraphPassAllocator& other) noexcept;
		RenderGraphPassAllocator& operator=(RenderGraphPassAllocator&& other) noexcept;

		typedef void(*PassExecFunc)(void*, const void*, RenderContext&);

		template<typename DataType, typename ExecFunc>
		Handle<RenderGraphPassNode<DataType>> AllocatePass(const std::string& name, ExecFunc&& execFunc)
		{
			auto passExecWrapperFunc = [](void* funcDataPtr, const void* passDataPtr, RenderContext& renderContext)
			{
				auto funcPtr = reinterpret_cast<ExecFunc*>(funcDataPtr);
				(*funcPtr)(*reinterpret_cast<const DataType*>(passDataPtr), renderContext);

				funcPtr->~ExecFunc();
			};

			auto passAllocation = AllocatePass(passExecWrapperFunc, sizeof(ExecFunc));
			new (passAllocation.executionFunctionPtr) ExecFunc(std::forward<ExecFunc>(execFunc));

			void* passNodeAllocation = m_passNodeAllocator.Allocate(sizeof(RenderGraphPassNode<DataType>));

			RenderGraphPassNode<DataType>* passNode = new(passNodeAllocation) RenderGraphPassNode<DataType>();
			passNode->name = name;
			passNode->passAllocationStartPtr = passAllocation.passAllocationStartPtr;
			passNode->index = m_numPasses;

			m_numPasses++;

			return passNode;
		}

		void ExecutePass(Handle<RenderGraphPassNodeBase> passNode, RenderContext& renderContext);

		VT_NODISCARD VT_INLINE uint32_t GetNumPasses() const { return m_numPasses; }

	private:
		inline static constexpr size_t MaxExecutionFunctionAllocationSize = 2 * 1024 * 1024;
		inline static constexpr size_t MaxPassNodeAllocationSize = 1 * 1024 * 1024;

		struct PassAllocation
		{
			void* executionFunctionPtr;
			void* passAllocationStartPtr;
		};

		PassAllocation AllocatePass(PassExecFunc execWrapperFunc, size_t execFuncSize);

		LinearAllocator<MaxExecutionFunctionAllocationSize> m_passExecutionFunctionAllocator;
		LinearAllocator<MaxPassNodeAllocationSize> m_passNodeAllocator;

		uint32_t m_numPasses = 0;
	};
}
