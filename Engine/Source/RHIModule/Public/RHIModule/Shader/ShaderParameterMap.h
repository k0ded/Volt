#pragma once

#include "RHIModule/Shader/ShaderCommon.h"

class BinaryStreamReader;
class BinaryStreamWriter;

namespace Volt::RHI
{
	class VTRHI_API ShaderParameterMap
	{
	public:
		using ResourceBindingsMap = Map<StringHash, ShaderResourceBinding>;
		using ParameterMap = Map<StringHash, ShaderUniform>;

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

		VT_NODISCARD VT_INLINE const ResourceBindingsMap& GetResourceBindings() const { return m_resourceBindings; }
		VT_NODISCARD VT_INLINE const ParameterMap& GetShaderParameters() const { return m_shaderParameters; }
		VT_NODISCARD VT_INLINE ShaderStage GetShaderStage() const { return m_shaderStage; }
		VT_NODISCARD VT_INLINE uint32_t GetShaderParametersSize() const { return m_shaderParameterSize; }
		VT_NODISCARD VT_INLINE bool AccessesRayTracingTable() const { return m_accessesRayTracingResourceTable; }

		static void Serialize(BinaryStreamWriter& streamWriter, const ShaderParameterMap& data);
		static void Deserialize(BinaryStreamReader& streamReader, ShaderParameterMap& outData);

	private:
		ResourceBindingsMap m_resourceBindings;
		ParameterMap m_shaderParameters;
		ShaderStage m_shaderStage;

		uint32_t m_shaderParameterSize = 0;
		bool m_accessesRayTracingResourceTable = false;
	};
}
