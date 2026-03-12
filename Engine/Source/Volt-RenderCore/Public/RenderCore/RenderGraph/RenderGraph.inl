#pragma once

namespace Volt
{
	template<typename ParameterStruct, typename ExecFunc>
	void RenderGraph::AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc)
	{
		VT_PROFILE_SCOPE(name.c_str());

		const ShaderParameterMetadataDescription* shaderParameterStructMetadata = ParameterStruct::GetShaderParameterMetadata();

		RGPassRef newPass = m_passAllocator.AllocatePass(name, std::forward<ExecFunc>(executeFunc), parameters, shaderParameterStructMetadata);
		newPass->m_flags = flags;
		m_renderPasses.emplace_back(newPass);

		SetupPass(newPass);
	}

	template<typename T>
	VT_INLINE T* RenderGraph::AllocParameters()
	{
		return m_passParametersAllocator.Allocate<T>();
	}

	VT_INLINE void* RenderGraph::AllocData(size_t size)
	{
		return m_dataAllocator.Get()->Allocate(size);
	}
}
