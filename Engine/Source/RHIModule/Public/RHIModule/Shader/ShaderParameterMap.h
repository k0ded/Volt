#pragma once

#include <CoreUtilities/Archive/Archive.h>

#include "RHIModule/Shader/ShaderCommon.h"

namespace Volt::RHI
{
	class VTRHI_API ShaderParameterMap
	{
	public:
		using ParameterMap = Map<StringHash, ShaderUniform>;

		struct ResourceBinding
		{
			ShaderResourceBinding binding;
			StringHash hash;

			friend Archive& operator<<(Archive& archive, ResourceBinding& value);
		};

		using ResourceBindings = Vector<ResourceBinding>;

		void AddUniformBuffer(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddSampler(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddStructuredBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddStructuredBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTexelBufferUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTexelBufferSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureSRV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureUAV(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddAccelerationStructure(const std::string& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);

		void AddParameter(const std::string& name, ShaderUniformType uniformType, uint32_t size, uint32_t offset);
		VT_NODISCARD VT_INLINE void SetAccessesRayTracingResourceTable() { m_accessesRayTracingResourceTable = true; }

		const ShaderUniform* GetParameterFromName(StringHash name) const;
		const ShaderResourceBinding* GetResourceBindingFromName(StringHash name) const;

		VT_INLINE void SetShaderStage(ShaderStage shaderStage) { m_shaderStage = shaderStage; }

		VT_NODISCARD VT_INLINE const ResourceBindings& GetResourceBindings() const { return m_resourceBindings; }
		VT_NODISCARD VT_INLINE const ParameterMap& GetShaderParameters() const { return m_shaderParameters; }
		VT_NODISCARD VT_INLINE ShaderStage GetShaderStage() const { return m_shaderStage; }
		VT_NODISCARD VT_INLINE uint32_t GetShaderParametersSize() const { return m_shaderParameterSize; }
		VT_NODISCARD VT_INLINE bool AccessesRayTracingTable() const { return m_accessesRayTracingResourceTable; }
		VT_NODISCARD VT_INLINE bool IsValid() const { return !m_resourceBindings.empty() || !m_shaderParameters.empty() || m_accessesRayTracingResourceTable; }
		VT_NODISCARD VT_INLINE bool HasShaderBindings() const { return !m_resourceBindings.empty(); }

		friend Archive& operator<<(Archive& archive, ShaderParameterMap& value);

	private:
		ResourceBindings m_resourceBindings;
		ParameterMap m_shaderParameters;
		ShaderStage m_shaderStage = ShaderStage::None;

		uint32_t m_shaderParameterSize = 0;
		bool m_accessesRayTracingResourceTable = false;
	};
}
