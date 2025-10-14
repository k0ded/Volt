#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Containers/ArrayView.h>

struct ID3D12RootSignature;

namespace Volt::RHI
{
	class D3D12Shader final : public Shader
	{
	public:
		D3D12Shader(const ShaderCreateInfo& createInfo);
		~D3D12Shader() override;

		void Reload(bool forceCompile /* = false */) override;
		std::string_view GetName() const override;
		size_t GetHash() const override;
		bool IsValid() const override;
		ShaderStage GetShaderStage() const override;
		const ShaderParameterMap& GetParameterMap() const override { return m_shaderParameterMap; }

		VT_NODISCARD VT_INLINE const ShaderInfo& GetShaderInfo() const override { return m_shaderInfo; }
		VT_NODISCARD VT_INLINE const ShaderSourceInfo& GetShaderSourceInfo() const override { return m_sourceInfo; }
		VT_NODISCARD VT_INLINE const ShaderIncludeDependencies& GetShaderIncludeDependencies() const override { return m_shaderIncludeDependencies; }
		VT_NODISCARD VT_INLINE ArrayView<uint32_t> GetShaderBinary() const { return m_shaderBinary; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void LoadAndCompileShader(bool forceCompile);
		void GenerateHash();

		ShaderParameterMap m_shaderParameterMap;
		ShaderPermutationConfig m_permutationConfig;
		ShaderSourceInfo m_sourceInfo;
		ShaderInfo m_shaderInfo;
		ShaderIncludeDependencies m_shaderIncludeDependencies;

		std::string m_name;
		size_t m_hash = 0;
		bool m_failureIsFatal;

		Vector<uint32_t> m_shaderBinary;
	};
}
