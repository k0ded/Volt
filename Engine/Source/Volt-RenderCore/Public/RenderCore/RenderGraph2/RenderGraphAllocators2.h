#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph2/RenderGraphPass.h"

#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>
#include <CoreUtilities/Containers/Vector.h>

#include <type_traits>

namespace Volt
{
	class RenderContext2;

	class VTRC_API RenderGraphResourceAllocator2
	{
	public:
		RenderGraphResourceAllocator2() = default;
		~RenderGraphResourceAllocator2();

		RenderGraphResourceAllocator2(const RenderGraphResourceAllocator2& other) noexcept = delete;
		RenderGraphResourceAllocator2(RenderGraphResourceAllocator2&& other) noexcept;
		RenderGraphResourceAllocator2& operator=(const RenderGraphResourceAllocator2& other) noexcept = delete;
		RenderGraphResourceAllocator2& operator=(RenderGraphResourceAllocator2&& other) noexcept;

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
		inline static constexpr size_t MaxResourceNodeAllocationSize = 512 * 1024;

		LinearAllocator<MaxResourceNodeAllocationSize> m_allocator;
		Vector<DestructorHelper> m_nodeDestructors;
	};
	
	class VTRC_API RenderGraphPassAllocator2
	{
	public:
		RenderGraphPassAllocator2() = default;
		~RenderGraphPassAllocator2();

		RenderGraphPassAllocator2(const RenderGraphPassAllocator2& other) noexcept = delete;
		RenderGraphPassAllocator2(RenderGraphPassAllocator2&& other) noexcept;
		RenderGraphPassAllocator2& operator=(const RenderGraphPassAllocator2& other) noexcept = delete;
		RenderGraphPassAllocator2& operator=(RenderGraphPassAllocator2&& other) noexcept;

		typedef void(*PassExecFunc)(void*, RenderContext2&);

		template<typename ExecFunc>
		Handle<RenderGraphPass> AllocatePass(const std::string& name, ExecFunc&& execFunc)
		{
			// Lmabda that will execute the pass
			auto passExecWrapperFunc = [](void* funcDataPtr, RenderContext2& renderContext)
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

		void ExecutePass(Handle<RenderGraphPass> pass, RenderContext2& renderContext);

		VT_NODISCARD VT_INLINE uint32_t GetNumPasses() const { return m_numPasses; }

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
}
