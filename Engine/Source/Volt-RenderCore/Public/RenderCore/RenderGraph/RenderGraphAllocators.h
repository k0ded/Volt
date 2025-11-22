#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/RenderGraphPass.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>
#include <CoreUtilities/Allocators/PagedAtomicLinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>
#include <CoreUtilities/Containers/Vector.h>

#include <type_traits>

namespace Volt
{
	class RenderContext;

	class VTRC_API RenderGraphResourceAllocator
	{
	public:
		RenderGraphResourceAllocator();
		~RenderGraphResourceAllocator();

		RenderGraphResourceAllocator(const RenderGraphResourceAllocator& other) noexcept = delete;
		RenderGraphResourceAllocator(RenderGraphResourceAllocator&& other) noexcept;
		RenderGraphResourceAllocator& operator=(const RenderGraphResourceAllocator& other) noexcept = delete;
		RenderGraphResourceAllocator& operator=(RenderGraphResourceAllocator&& other) noexcept;

		template<typename ResourceType, typename... Args>
		ResourceType* Allocate(Args&&... args)
		{
			constexpr size_t allocationSize = sizeof(ResourceType);

			void* allocationPtr = m_allocator.Allocate(allocationSize);
			ResourceType* newResource = new (allocationPtr) ResourceType(std::forward<Args>(args)...);

			// Destructor for the allocated resource object, required because we are using the linear allocator.
			m_nodeDestructors.emplace_back() = DestructorHelper::Create<ResourceType>(allocationPtr);
			return newResource;
		}

	private:
		PagedAtomicLinearAllocator<65536> m_allocator;
		Vector<DestructorHelper> m_nodeDestructors;
	};
	
	class VTRC_API RenderGraphPassAllocator
	{
	public:
		RenderGraphPassAllocator();
		~RenderGraphPassAllocator();

		RenderGraphPassAllocator(const RenderGraphPassAllocator& other) noexcept = delete;
		RenderGraphPassAllocator(RenderGraphPassAllocator&& other) noexcept;
		RenderGraphPassAllocator& operator=(const RenderGraphPassAllocator& other) noexcept = delete;
		RenderGraphPassAllocator& operator=(RenderGraphPassAllocator&& other) noexcept;

		typedef void(*PassExecFunc)(void*, RenderContext&);

		template<typename ExecFunc>
		Handle<RenderGraphPass> AllocatePass(const std::string& name, ExecFunc&& execFunc)
		{
			// Lmabda that will execute the pass
			auto passExecWrapperFunc = [](void* funcDataPtr, RenderContext& renderContext)
			{
				auto funcPtr = reinterpret_cast<ExecFunc*>(funcDataPtr);
				(*funcPtr)(renderContext);

				funcPtr->~ExecFunc();
			};

			auto passAllocation = AllocatePass(passExecWrapperFunc, sizeof(ExecFunc));
			new (passAllocation.executionFunctionPtr) ExecFunc(std::forward<ExecFunc>(execFunc));

			void* passNodeAllocation = m_passNodeAllocator.Allocate(sizeof(RenderGraphPass));

			// Destructor for the allocated pass object, required because we are using the linear allocator.
			m_passDestructors.emplace_back() = DestructorHelper::Create<RenderGraphPass>(passNodeAllocation);

			RenderGraphPass* passNode = new(passNodeAllocation) RenderGraphPass();
			passNode->name = name;
			passNode->passAllocationStartPtr = passAllocation.passAllocationStartPtr;
			passNode->passIndex = m_numPasses;

			m_numPasses++;

			return passNode;
		}

		void ExecutePass(Handle<RenderGraphPass> pass, RenderContext& renderContext);

		VT_NODISCARD VT_INLINE uint32_t GetNumPasses() const { return m_numPasses; }

		struct PassAllocation
		{
			void* executionFunctionPtr;
			void* passAllocationStartPtr;
		};

		PassAllocation AllocatePass(PassExecFunc execWrapperFunc, size_t execFuncSize);

		PagedAtomicLinearAllocator<65536> m_passExecutionFunctionAllocator;
		PagedAtomicLinearAllocator<65536> m_passNodeAllocator;

		uint32_t m_numPasses = 0;
		Vector<DestructorHelper> m_passDestructors;
	};
}
