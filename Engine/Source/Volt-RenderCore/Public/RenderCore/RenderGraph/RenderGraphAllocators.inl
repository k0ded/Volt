#pragma once

namespace Volt
{
	template<typename ExecFunc, typename T>
	RenderGraphPassRef RenderGraphPassAllocator::AllocatePass(const std::string& name, ExecFunc&& execFunc, const T* shaderParameters, const ShaderParameterMetadataDescription* shaderParameterMetadata)
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

		RenderGraphPass* passNode = new(passNodeAllocation) RenderGraphPass(shaderParameters, shaderParameterMetadata);
		passNode->name = name;
		passNode->passAllocationStartPtr = passAllocation.passAllocationStartPtr;
		passNode->passIndex = m_numPasses;

		m_numPasses++;

		return passNode;
	}

}
