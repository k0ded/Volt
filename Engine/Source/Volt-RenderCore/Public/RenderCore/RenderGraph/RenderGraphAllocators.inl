#pragma once

namespace Volt
{
	template<typename ExecFunc, typename T>
	RGPassRef RenderGraphPassAllocator::AllocatePass(const std::string& name, ExecFunc&& execFunc, const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata)
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

		void* passNodeAllocation = m_passNodeAllocator.Allocate(sizeof(RGPass));

		// Destructor for the allocated pass object, required because we are using the linear allocator.
		m_passDestructors.emplace_back() = DestructorHelper::Create<RGPass>(passNodeAllocation);

		RGPass* passNode = new(passNodeAllocation) RGPass(shaderParameters, shaderParameterMetadata, m_dataAllocator);
		passNode->m_name = name;
		passNode->m_passAllocationStartPtr = passAllocation.passAllocationStartPtr;
		passNode->passIndex = m_numPasses;

		m_numPasses++;

		return passNode;
	}

	template<typename ResourceType, typename... Args>
	ResourceType* RenderGraphResourceAllocator::Allocate(Args&&... args)

	{
		constexpr size_t allocationSize = sizeof(ResourceType);

		void* allocationPtr = m_allocator.Allocate(allocationSize);
		ResourceType* newResource = new (allocationPtr) ResourceType(std::forward<Args>(args)...);

		// Destructor for the allocated resource object, required because we are using the linear allocator.
		m_nodeDestructors.emplace_back() = DestructorHelper::Create<ResourceType>(allocationPtr);
		return newResource;
	}
}
