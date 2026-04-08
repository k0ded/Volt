#pragma once

#include <CoreUtilities/Archive/Archive.h>

#include "RHIModule/Shader/ShaderCommon.h"

namespace Volt::RHI
{
	class VTRHI_API ShaderParameterMap
	{
	public:
		struct ResourceBinding
		{
			ShaderResourceBinding binding;
			StringHash hash;

			friend Archive& operator<<(Archive& archive, ResourceBinding& value);
		};

		using ParameterMap = Map<StringHash, ShaderUniform>;
		using ResourceBindings = Vector<ResourceBinding>;
		using InlineParameterBlock = Map<StringHash, ShaderUniform>;

		void AddUniformBuffer(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddSampler(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddStructuredBufferUAV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddStructuredBufferSRV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTexelBufferUAV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTexelBufferSRV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureSRV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddTextureUAV(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);
		void AddAccelerationStructure(const String& name, uint32_t set, uint32_t binding, ShaderStage shaderStage);

		void AddParameter(const String& name, ShaderUniformType uniformType, uint32_t size, uint32_t offset);
		void AddInlineParameter(const String& name, ShaderUniformType uniformType, uint32_t size, uint32_t offset);

		const ShaderUniform* GetParameterFromName(StringHash name) const;
		const ShaderResourceBinding* GetResourceBindingFromName(StringHash name) const;

		VT_INLINE void SetShaderStage(ShaderStage shaderStage) { m_shaderStage = shaderStage; }
		VT_INLINE void SetAccessesResourceTable() { m_accessesResourceTable = true; }

		VT_NODISCARD VT_INLINE const ResourceBindings& GetResourceBindings() const { return m_resourceBindings; }
		VT_NODISCARD VT_INLINE const ParameterMap& GetShaderParameters() const { return m_shaderParameters; }
		VT_NODISCARD VT_INLINE const InlineParameterBlock& GetInlineParameterBlock() const { return m_inlineParameterBlock; }
		VT_NODISCARD VT_INLINE ShaderStage GetShaderStage() const { return m_shaderStage; }
		VT_NODISCARD VT_INLINE uint32_t GetShaderParametersSize() const { return m_shaderParameterSize; }
		VT_NODISCARD VT_INLINE uint32_t GetInlineParameterBlockSize() const { return m_inlineParameterBlockSize; }
		VT_NODISCARD VT_INLINE bool AccessesResourceTable() const { return m_accessesResourceTable; }
		VT_NODISCARD VT_INLINE bool IsValid() const { return !m_resourceBindings.empty() || !m_shaderParameters.empty() || m_accessesResourceTable; }
		VT_NODISCARD VT_INLINE bool HasShaderBindings() const { return !m_resourceBindings.empty(); }
		VT_NODISCARD VT_INLINE bool HasInlineParameterBlock() const { return !m_inlineParameterBlock.empty(); }

		friend Archive& operator<<(Archive& archive, ShaderParameterMap& value);

	private:
		ResourceBindings m_resourceBindings;
		ParameterMap m_shaderParameters;

		InlineParameterBlock m_inlineParameterBlock;

		ShaderStage m_shaderStage = ShaderStage::None;

		uint32_t m_shaderParameterSize = 0;
		uint32_t m_inlineParameterBlockSize = 0;
		bool m_accessesResourceTable = false;
	};
}
