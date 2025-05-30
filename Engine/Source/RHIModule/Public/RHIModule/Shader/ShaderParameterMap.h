#pragma once

#include "RHIModule/Shader/ShaderCommon.h"

namespace Volt::RHI
{
	class VTRHI_API ShaderParameterMap
	{
	public:
		using ResourceBindingsMap = vt::map<StringHash, ShaderResourceBinding>;
		using ParameterMap = vt::map<StringHash, ShaderUniform>;

		void AddUniformBuffer(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddSampler(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddBufferUAV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddBufferSRV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureSRV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureUAV(std::string_view name, uint32_t set, uint32_t binding, ShaderStage shaderStage);

		void AddParameter(std::string_view name, ShaderUniformType uniformType, uint32_t size, uint32_t offset);

		const ShaderUniform* GetParameterFromName(StringHash name) const;
		const ShaderResourceBinding* GetResourceBindingFromName(StringHash name) const;

		VT_INLINE void SetShaderStage(ShaderStage shaderStage) { m_shaderStage = shaderStage; }

		VT_NODISCARD VT_INLINE const ResourceBindingsMap& GetResourceBindings() const { return m_resourceBindings; }
		VT_NODISCARD VT_INLINE const ParameterMap& GetShaderParameters() const { return m_shaderParameters; }
		VT_NODISCARD VT_INLINE ShaderStage GetShaderStage() const { return m_shaderStage; }
		VT_NODISCARD VT_INLINE uint32_t GetShaderParametersSize() const { return m_shaderParameterSize; }

	private:
		ResourceBindingsMap m_resourceBindings;
		ParameterMap m_shaderParameters;
		ShaderStage m_shaderStage;

		uint32_t m_shaderParameterSize = 0;
	};
}
