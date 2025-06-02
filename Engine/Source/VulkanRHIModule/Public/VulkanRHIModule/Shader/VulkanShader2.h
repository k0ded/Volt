#pragma once

#include <RHIModule/Shader/Shader2.h>
#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Shader/BufferLayout.h>

struct VkShaderModule_T;

namespace Volt::RHI
{
	class VulkanShader2 final : public Shader2
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

			std::map<std::string, ShaderResourceBinding> bindings;
		};

		VulkanShader2(const ShaderCreateInfo& createInfo);
		~VulkanShader2() override;

		std::string_view GetName() const override;
		size_t GetHash() const override;
		bool IsValid() const override;
		ShaderStage GetShaderStage() const override;

		VT_NODISCARD VT_INLINE const ShaderBindings& GetBindings() const override { return m_bindings; }
		VT_NODISCARD VT_INLINE const ShaderInfo& GetShaderInfo() const { return m_shaderInfo; }
		VT_NODISCARD VT_INLINE const ShaderSourceInfo& GetShaderSourceInfo() const { return m_sourceInfo; }
		VT_NODISCARD VT_INLINE VkShaderModule_T* GetShaderModule() const { return m_shaderModule; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void LoadAndCompileShader();
		void CreateShader(const Vector<uint32_t>& shaderBinary);

		ShaderPermutationConfig m_permutationConfig;
		ShaderSourceInfo m_sourceInfo;
		ShaderBindings m_bindings;
		ShaderInfo m_shaderInfo;

		VkShaderModule_T* m_shaderModule = nullptr;
		std::string m_name;
	};
}
