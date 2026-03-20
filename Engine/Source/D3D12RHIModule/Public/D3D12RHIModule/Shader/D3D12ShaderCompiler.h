#pragma once

#include <RHIModule/Core/RHICommon.h>
#include <RHIModule/Shader/ShaderCompiler.h>

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcRewriter;
struct IDxcRewriter2;
struct IDxcResult;
struct ID3D12ShaderReflection;

namespace Volt::RHI
{
	class HLSLIncluder;

	class D3D12ShaderCompiler final : public ShaderCompiler
	{
	public:
		D3D12ShaderCompiler(const ShaderCompilerCreateInfo& createInfo);
		~D3D12ShaderCompiler() override;

	protected:
		CompilationResultData TryCompileImpl(const Specification& specification) override;
		void AddMacroImpl(const std::string& macroName) override;
		void RemoveMacroImpl(std::string_view macroName) override;
		void* GetHandleImpl() const override;

	private:
		struct DxcCompilationResult
		{
			IDxcResult* dxcResult;
			ID3D12ShaderReflection* reflectionData;
			std::string error;
			bool succeded;
		};

		struct RewriteResult
		{
			std::string outSource;
			std::string error;
			bool succeded;
		};

		CompilationResultData CompileShader(const Specification& specification, ID3D12ShaderReflection*& reflectionData);
		bool PreprocessSource(const Specification& specification, std::string& outProcessedSource, CompilationResultData& compilationResult);
		void ReflectShader(const Specification& specification, CompilationResultData& inOutData, ID3D12ShaderReflection* reflectionData);

		DxcCompilationResult InvokeCompilerWithArguments(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source, HLSLIncluder* includer);
		RewriteResult RewriteHLSL(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source);

		IDxcCompiler3* m_hlslCompiler = nullptr;
		IDxcUtils* m_hlslUtils = nullptr;
		IDxcRewriter* m_hlslRewriter = nullptr;
		IDxcRewriter2* m_hlslRewriter2 = nullptr;

		Vector<std::filesystem::path> m_includeDirectories;
		Vector<std::string> m_macros;
		ShaderCompilerFlags m_flags = ShaderCompilerFlags::None;
		std::filesystem::path m_cacheDirectory;

		IntRef<ShaderCache> m_shaderCache;
	};
}
