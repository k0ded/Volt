#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/RenderGraphPass.h"
#include "RenderCore/RenderGraph/RenderGraphDataAllocator.h"

#include <CoreUtilities/DestructorHelper.h>
#include <CoreUtilities/Containers/Vector.h>

#include <type_traits>

namespace Volt
{
	class RenderContext;
	class ShaderParameterMetadataDescription;

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
		ResourceType* Allocate(Args&&... args);

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

		template<typename ExecFunc, typename T>
		RGPassRef AllocatePass(const std::string& name, ExecFunc&& execFunc, const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata);

		void ExecutePass(RGPassRef pass, RenderContext& renderContext);

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

#include "RenderGraphAllocators.inl"
