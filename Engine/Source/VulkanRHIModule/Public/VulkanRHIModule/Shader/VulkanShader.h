#pragma once

#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Shader/BufferLayout.h>

struct VkShaderModule_T;

namespace Volt::RHI
{
	class VulkanShader final : public Shader
	{
	public:
		struct ShaderInfo
		{
			// Pixel Shader
			Vector<RHI::PixelFormat> outputFormats;

			// Vertex Shader
			RHI::BufferLayout vertexLayout;
			RHI::BufferLayout instanceLayout;

			// Common
			ShaderUniforms shaderUniforms{};
		};

		VulkanShader(const ShaderCreateInfo& createInfo);
		~VulkanShader() override;

		void Reload(bool forceCompile /* = false */) override;
		std::string_view GetName() const override;
		size_t GetHash() const override;
		bool IsValid() const override;
		ShaderStage GetShaderStage() const override;
		const ShaderParameterMap& GetParameterMap() const override { return m_shaderParameterMap; }

		VT_NODISCARD VT_INLINE const ShaderInfo& GetShaderInfo() const { return m_shaderInfo; }
		VT_NODISCARD VT_INLINE const ShaderSourceInfo& GetShaderSourceInfo() const { return m_sourceInfo; }
		VT_NODISCARD VT_INLINE VkShaderModule_T* GetShaderModule() const { return m_shaderModule; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void LoadAndCompileShader(bool forceCompile);
		void CreateShader(const Vector<uint32_t>& shaderBinary);
		void GenerateHash();

		ShaderParameterMap m_shaderParameterMap;
		ShaderPermutationConfig m_permutationConfig;
		ShaderSourceInfo m_sourceInfo;
		ShaderInfo m_shaderInfo;

		VkShaderModule_T* m_shaderModule = nullptr;
		std::string m_name;
		size_t m_hash = 0;
	};
}
