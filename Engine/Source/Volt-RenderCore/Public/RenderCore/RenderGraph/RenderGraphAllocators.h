#pragma once

#include "RenderCore/RenderGraph/RenderGraphPass.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>

namespace Volt
{
	class VTRC_API RenderGraphPassAllocator
	{
	public:
		RenderGraphPassAllocator() = default;
		~RenderGraphPassAllocator();

		RenderGraphPassAllocator(const RenderGraphPassAllocator& other) noexcept = delete;
		RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept;
		RenderGraphPassAllocator& operator=(const RenderGraphPassAllocator& other) noexcept = delete;
		RenderGraphPassAllocator& operator=(RenderGraphPassAllocator&& other) noexcept;

		typedef void(*PassExecFunc)(void*, const void*, RenderContext&);

		template<typename DataType, typename ExecFunc>
		Handle<RenderGraphPassNode<DataType>> AllocatePass(const std::string& name, ExecFunc&& execFunc)
		{
			// Lmabda that will execute the pass
			auto passExecWrapperFunc = [](void* funcDataPtr, const void* passDataPtr, RenderContext& renderContext)
			{
				auto funcPtr = reinterpret_cast<ExecFunc*>(funcDataPtr);
				(*funcPtr)(*reinterpret_cast<const DataType*>(passDataPtr), renderContext);

				funcPtr->~ExecFunc();
			};

			auto passAllocation = AllocatePass(passExecWrapperFunc, sizeof(ExecFunc));
			new (passAllocation.executionFunctionPtr) ExecFunc(std::forward<ExecFunc>(execFunc));

			void* passNodeAllocation = m_passNodeAllocator.Allocate(sizeof(RenderGraphPassNode<DataType>));

			// Destructor for the allocated pass object, required because we are using the linear allocator.
			m_passDestructors.emplace_back() = DestructorHelper::Create<RenderGraphPassNode<DataType>>(passNodeAllocation);

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
		Vector<DestructorHelper> m_passDestructors;
	};

	class VTRC_API RenderGraphResourceNodeAllocator
	{
	public:
		RenderGraphResourceNodeAllocator() = default;
		~RenderGraphResourceNodeAllocator();

		RenderGraphResourceNodeAllocator(const RenderGraphResourceNodeAllocator& other) noexcept = delete;
		RenderGraphResourceNodeAllocator(RenderGraphResourceNodeAllocator&& other) noexcept;
		RenderGraphResourceNodeAllocator& operator=(const RenderGraphResourceNodeAllocator& other) noexcept = delete;
		RenderGraphResourceNodeAllocator& operator=(RenderGraphResourceNodeAllocator&& other) noexcept;

		template<typename ResourceType>
		Handle<RenderGraphResourceNode<ResourceType>> Allocate()
		{
			constexpr size_t allocationSize = sizeof(RenderGraphResourceNode<ResourceType>);

			void* allocationPtr = m_allocator.Allocate(allocationSize);
			RenderGraphResourceNode<ResourceType>* newNode = new (allocationPtr) RenderGraphResourceNode<ResourceType>();

			// Destructor for the allocated pass object, required because we are using the linear allocator.
			m_nodeDestructors.emplace_back() = DestructorHelper::Create<RenderGraphResourceNode<ResourceType>>(allocationPtr);

			newNode->handle = *reinterpret_cast<RenderGraphResourceHandle*>(&m_numResourceNodes);
			m_numResourceNodes++;

			return newNode;
		}

		VT_NODISCARD VT_INLINE uint32_t GetNumResourceNodes() const { return m_numResourceNodes; }
		VT_NODISCARD VT_INLINE uint32_t GetAndIncrementHandle() { uint32_t value = m_numResourceNodes; m_numResourceNodes++; return value; }

	private:
		inline static constexpr size_t MaxResourceNodeAllocationSize = 512 * 1024;
	
		LinearAllocator<MaxResourceNodeAllocationSize> m_allocator;

		uint32_t m_numResourceNodes = 0;
		Vector<DestructorHelper> m_nodeDestructors;
	};
}
