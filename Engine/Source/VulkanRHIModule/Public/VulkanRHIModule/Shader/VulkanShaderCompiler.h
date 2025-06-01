#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Shader/ShaderCompiler.h>

struct IDxcCompiler3;
struct IDxcUtils;

namespace Volt::RHI
{
	class VulkanShaderCompiler final : public ShaderCompiler
	{
	public:
		VulkanShaderCompiler(const ShaderCompilerCreateInfo& createInfo);
		~VulkanShaderCompiler() override;

	protected:
		CompilationResultData2 TryCompileImpl2(const Specification2& specification) override;
		void AddMacroImpl(const std::string& macroName) override;
		void RemoveMacroImpl(std::string_view macroName) override;
		void* GetHandleImpl() const override;

	private:
		CompilationResultData2 CompileShader(const Specification2& specification);
		bool PreprocessSource2(const Specification2& specification, std::string& outProcessedSource);
		void ReflectShader(const Specification2& specification, CompilationResultData2& inOutData);

		IDxcCompiler3* m_hlslCompiler = nullptr;
		IDxcUtils* m_hlslUtils = nullptr;
	
		Vector<std::filesystem::path> m_includeDirectories;
		Vector<std::string> m_macros;
		ShaderCompilerFlags m_flags = ShaderCompilerFlags::None;
		std::filesystem::path m_cacheDirectory;

		RefPtr<ShaderCache> m_shaderCache;
	};
}
