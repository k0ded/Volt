#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Shader/ShaderCompiler.h>

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcRewriter;
struct IDxcRewriter2;
struct IDxcResult;

namespace Volt::RHI
{
	class HLSLIncluder;

	class VulkanShaderCompiler final : public ShaderCompiler
	{
	public:
		VulkanShaderCompiler(const ShaderCompilerCreateInfo& createInfo);
		~VulkanShaderCompiler() override;

	protected:
		CompilationResultData TryCompileImpl(const Specification& specification) override;
		void AddMacroImpl(const String& macroName) override;
		void RemoveMacroImpl(StringView macroName) override;
		void* GetHandleImpl() const override;

	private:
		struct DxcCompilationResult
		{
			IDxcResult* dxcResult;
			String error;
			bool succeded;
		};

		CompilationResultData CompileShader(const Specification& specification);
		bool PreprocessSource(const Specification& specification, String& outProcessedSource, CompilationResultData& compilationResult);

		void OptimizeSpirvForReflection(const Specification& specification, CompilationResultData& inOutData, Vector<uint32_t>& outSpirv);
		void ReflectAndRewriteSpirv(ShaderStage currentShaderStage, Vector<uint32_t>& spirv, ShaderParameterMap& shaderParameterMap);
		void ReflectShader(const Specification& specification, CompilationResultData& inOutData);

		void DumpSpirv(const Specification& specification, const CompilationResultData& data);
		void DumpShaderText(const Specification& specification, StringView shaderText);
		Filesystem::Path GetShaderDumpDirectory(const Specification& specification) const;

		DxcCompilationResult InvokeCompilerWithArguments(Vector<const wchar_t*>& arguments, const Filesystem::Path& sourceFilepath, const String& source, HLSLIncluder* includer);

		bool ShouldDumpShaderDebugInfo() const;

		IDxcCompiler3* m_hlslCompiler = nullptr;
		IDxcUtils* m_hlslUtils = nullptr;
	
		ShaderCompilerCreateInfo m_createInfo;

		Vector<Filesystem::Path> m_includeDirectories;
		Vector<String> m_macros;

		IntRef<ShaderCache> m_shaderCache;
	};
}
