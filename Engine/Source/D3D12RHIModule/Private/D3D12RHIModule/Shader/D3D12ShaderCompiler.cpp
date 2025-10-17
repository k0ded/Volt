#include "dxpch.h"

#include "D3D12RHIModule/Shader/D3D12ShaderCompiler.h"
#include "D3D12RHIModule/Shader/HLSLIncluder.h"

#include <RHIModule/Shader/ShaderUtility.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Shader/ShaderPreProcessor.h>
#include <RHIModule/Shader/ShaderCache.h>
#include <RHIModule/Globals.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/StringUtility.h>

#ifdef _WIN32
#include <wrl.h>
#else
#include <dxsc/WinAdapter.h>
#endif

#include <dxc/dxcapi.h>
#include <dxc/dxctools.h>

#include <d3d12/d3d12shader.h>

namespace Volt::RHI
{
	namespace Utility
	{
		inline static const std::string GetErrorStringFromResult(IDxcResult* result)
		{
			std::string output;

			IDxcBlobUtf8* errors;
			result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
			if (errors && errors->GetStringLength() > 0)
			{
				output = (char*)errors->GetBufferPointer();
				errors->Release();
			}

			return output;
		}

		inline static const std::string GetErrorStringFromResult(IDxcOperationResult* result)
		{
			IDxcBlobEncoding* error;
			result->GetErrorBuffer(&error);

			return std::string(reinterpret_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
		}

		inline static ShaderUniformType GetShaderUniformTypeFromD3D12TypeDesc(const D3D12_SHADER_TYPE_DESC& typeDesc)
		{
			ShaderUniformType resultType;

			switch (typeDesc.Type)
			{
				case D3D_SVT_BOOL: resultType.baseType = ShaderUniformBaseType::Bool; break;
				case D3D_SVT_UINT64: resultType.baseType = ShaderUniformBaseType::UInt64; break;
				case D3D_SVT_INT64: resultType.baseType = ShaderUniformBaseType::Int64; break;
				case D3D_SVT_UINT: resultType.baseType = ShaderUniformBaseType::UInt; break;
				case D3D_SVT_INT: resultType.baseType = ShaderUniformBaseType::Int; break;
				case D3D_SVT_FLOAT: resultType.baseType = ShaderUniformBaseType::Float; break;
				case D3D_SVT_DOUBLE: resultType.baseType = ShaderUniformBaseType::Double; break;
				case D3D_SVT_FLOAT16: resultType.baseType = ShaderUniformBaseType::Half; break;
				case D3D_SVT_INT16: resultType.baseType = ShaderUniformBaseType::Short; break;
				case D3D_SVT_UINT16: resultType.baseType = ShaderUniformBaseType::UShort; break;
			}

			// #TODO_Ivar: Is this correct?
			resultType.columns = typeDesc.Rows;
			resultType.vecsize = typeDesc.Columns;

			return resultType;
		}
	}

	D3D12ShaderCompiler::D3D12ShaderCompiler(const ShaderCompilerCreateInfo& createInfo)
		: m_includeDirectories(createInfo.includeDirectories), m_macros(createInfo.initialMacros), m_flags(createInfo.flags),
		m_shaderCache(createInfo.shaderCache)
	{
		VT_LOGC(Trace, LogD3D12RHI, "Initializing D3D12ShaderCompiler");
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_hlslCompiler));
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_hlslUtils));
		DxcCreateInstance(CLSID_DxcRewriter, IID_PPV_ARGS(&m_hlslRewriter));

		m_hlslRewriter->QueryInterface(&m_hlslRewriter2);
	}

	D3D12ShaderCompiler::~D3D12ShaderCompiler()
	{
		m_hlslUtils->Release();
		m_hlslCompiler->Release();
		m_hlslRewriter->Release();
		m_hlslRewriter2->Release();

		VT_LOGC(Trace, LogD3D12RHI, "Destroying D3D12ShaderCompiler");
	}

	void D3D12ShaderCompiler::AddMacroImpl(const std::string& macroName)
	{
		if (std::find(m_macros.begin(), m_macros.end(), macroName) != m_macros.end())
		{
			return;
		}

		m_macros.push_back(macroName);
	}

	void D3D12ShaderCompiler::RemoveMacroImpl(std::string_view macroName)
	{
		if (auto it = std::find(m_macros.begin(), m_macros.end(), macroName); it != m_macros.end())
		{
			m_macros.erase(it);
		}
	}

	void* D3D12ShaderCompiler::GetHandleImpl() const
	{
		return nullptr;
	}

	ShaderCompiler::CompilationResultData D3D12ShaderCompiler::TryCompileImpl(const Specification& specification)
	{
		if (!specification.forceCompile)
		{
			const auto cachedResult = m_shaderCache->TryGetCachedShader(specification);
			if (cachedResult.data.IsValid())
			{
				return cachedResult.data;
			}
		}

		if (specification.shaderSourceInfo.source.empty())
		{
			VT_LOGC(Error, LogD3D12RHI, "Trying to compile a shader without a source!");
			return {};
		}

		ID3D12ShaderReflection* reflectionData = nullptr;

		CompilationResultData result = CompileShader(specification, reflectionData);
		if (result.result != ShaderCompiler::CompilationResult::Success)
		{
			const auto cachedResult = m_shaderCache->TryGetCachedShader(specification);
			return cachedResult.data;
		}

		if (reflectionData)
		{
			ReflectShader(specification, result, reflectionData);
			reflectionData->Release();
		}

		m_shaderCache->CacheShader(specification, result);

		return result;
	}

	ShaderCompiler::CompilationResultData D3D12ShaderCompiler::CompileShader(const Specification& specification, ID3D12ShaderReflection*& reflectionData)
	{
		CompilationResultData result;

		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;
		std::string processedSource = specification.shaderSourceInfo.source;

		if (!PreprocessSource(specification, processedSource, result))
		{
			result.result = ShaderCompiler::CompilationResult::PreprocessFailed;
			return result;
		}

		const std::wstring wEntryPoint = ::Utility::ToWString(sourceEntry.entryPoint);
		const std::wstring globalsBinding = std::to_wstring(Globals::SHADER_GLOBALS_BINDING);
		const std::wstring globalsSpace = std::to_wstring(Globals::SHADER_GLOBALS_SPACE);

		Vector<const wchar_t*> arguments =
		{
			sourceEntry.filepath.c_str(),
			L"-E",
			wEntryPoint.c_str(),
			L"-T",
			Utility::HLSLShaderProfile(sourceEntry.shaderStage),
			L"-HV",
			L"2021",
			L"-D", L"__D3D12__ ",

			DXC_ARG_PACK_MATRIX_COLUMN_MAJOR
		};

		// Append permutations
		const Vector<std::wstring> permutationStrings = specification.permutationConfig.GetPermutationsWideStr();
		for (const auto& permutationStr : permutationStrings)
		{
			arguments.push_back(L"-D");
			arguments.push_back(permutationStr.c_str());
		}

		if (g_rhiCapabilities.supportsNative16BitOperations)
		{
			arguments.push_back(L"-enable-16bit-types");
		}

		if ((m_flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		switch (specification.optimizationLevel)
		{
			case ShaderCompiler::OptimizationLevel::Disable: arguments.push_back(L"-Od"); break;
			case ShaderCompiler::OptimizationLevel::Release: arguments.push_back(L"-O1"); break;
			case ShaderCompiler::OptimizationLevel::Dist: arguments.push_back(L"-O3"); break;
		}

		if (specification.optimizationLevel != ShaderCompiler::OptimizationLevel::Dist)
		{
			arguments.push_back(DXC_ARG_DEBUG);
			arguments.push_back(L"-Qembed_debug");
		}

		// Compile
		DxcCompilationResult compilationResult = InvokeCompilerWithArguments(arguments, sourceEntry.filepath, processedSource, nullptr);

		if (compilationResult.succeded)
		{
			IDxcBlob* shaderResult = nullptr;
			compilationResult.dxcResult->GetResult(&shaderResult);

			const size_t size = shaderResult->GetBufferSize();

			result.shaderBinary.resize_uninitialized(size / sizeof(uint32_t));
			memcpy_s(result.shaderBinary.data(), size, shaderResult->GetBufferPointer(), shaderResult->GetBufferSize());

			shaderResult->Release();
			result.result = ShaderCompiler::CompilationResult::Success;

			VT_LOGC(Info, LogD3D12RHI, "Successfully compiled shader {}!", sourceEntry.filepath);
		}
		else
		{
			VT_LOGC_UNFORMATTED(Error, LogD3D12RHI, compilationResult.error);
			result.result = ShaderCompiler::CompilationResult::Failure;
		}

		if (compilationResult.dxcResult)
		{
			compilationResult.dxcResult->Release();
		}

		reflectionData = compilationResult.reflectionData;
		return result;
	}

	bool D3D12ShaderCompiler::PreprocessSource(const Specification& specification, std::string& outProcessedSource, CompilationResultData& compilationResult)
	{
		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;

		Vector<std::wstring> wIncludeDirs;
		Vector<const wchar_t*> wcIncludeDirs;

		// Add platform include
		constexpr std::string_view platformInclude = "#include \"Platforms/D3D12/D3D12Interop.hlsli\"\n";
		outProcessedSource.insert(outProcessedSource.begin(), platformInclude.begin(), platformInclude.end());

		for (const auto& includeDir : m_includeDirectories)
		{
			wIncludeDirs.push_back(L"-I " + includeDir.wstring());
		}

		for (const auto& includeDir : wIncludeDirs)
		{
			wcIncludeDirs.push_back(includeDir.c_str());
		}

		Vector<const wchar_t*> definesAndIncludes =
		{
			L"-D", L"__HLSL__",
			L"-D", L"__D3D12__"
		};

		// Append permutations
		Vector<std::wstring> permutationStrings = specification.permutationConfig.GetPermutationsWideStr();
		for (const auto& permutationStr : permutationStrings)
		{
			definesAndIncludes.push_back(L"-D");
			definesAndIncludes.push_back(permutationStr.c_str());
		}

		// Append include dirs
		for (const auto& includeDir : wcIncludeDirs)
		{
			definesAndIncludes.push_back(includeDir);
		}

		// Append global macros
		Vector<std::wstring> wMacros;
		for (const auto& macro : m_macros)
		{
			wMacros.push_back(::Utility::ToWString(macro));
		}

		for (const auto& macro : wMacros)
		{
			definesAndIncludes.emplace_back(L"-D");
			definesAndIncludes.emplace_back(macro.c_str());
		}

		// Append compile flags
		if ((m_flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			definesAndIncludes.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		bool succeded = false;

		// Pre process source
		{
			Vector<const wchar_t*> compilationArgs =
			{
				sourceEntry.filepath.c_str(),
				L"-P", // Preprocess
			};
			compilationArgs.append(definesAndIncludes);

			const Scope<HLSLIncluder> includer = CreateScope<HLSLIncluder>();
			DxcCompilationResult preProcessingResult = InvokeCompilerWithArguments(compilationArgs, sourceEntry.filepath, outProcessedSource, includer.get());

			for (const auto& filepath : includer->GetIncludedFiles())
			{
				compilationResult.includeDependencies.emplace_back(filepath);
			}

			if (preProcessingResult.succeded)
			{
				IDxcBlob* blob = nullptr;
				preProcessingResult.dxcResult->GetResult(&blob);

				outProcessedSource = reinterpret_cast<const char*>(blob->GetBufferPointer());
				blob->Release();
			}
			else
			{
				VT_LOGC_UNFORMATTED(Error, LogD3D12RHI, preProcessingResult.error);
			}

			if (preProcessingResult.dxcResult)
			{
				preProcessingResult.dxcResult->Release();
			}

			succeded = preProcessingResult.succeded;
		}

		// Custom pre processing
		if (succeded)
		{
			PreProcessorData processingData{};
			processingData.shaderSource = outProcessedSource;
			processingData.shaderStage = sourceEntry.shaderStage;
			processingData.entryPoint = sourceEntry.entryPoint;

			PreProcessorResult preProcessorResult{};
			if (!ShaderPreProcessor::PreProcessShaderSource(processingData, preProcessorResult))
			{
				compilationResult.result = CompilationResult::PreprocessFailed;
				succeded = false;
			}

			if (sourceEntry.shaderStage == ShaderStage::Pixel)
			{
				compilationResult.outputFormats = preProcessorResult.outputFormats;
			}
			else if (sourceEntry.shaderStage == ShaderStage::Vertex)
			{
				compilationResult.vertexLayout = preProcessorResult.vertexLayout;
				compilationResult.instanceLayout = preProcessorResult.instanceLayout;
			}

			outProcessedSource = preProcessorResult.preProcessedResult;
		}

		// Rewrite source
#if 0
		if (succeded)
		{
			const std::wstring wEntryPoint = ::Utility::ToWString(sourceEntry.entryPoint);

			Vector<const wchar_t*> rewriteArgs =
			{
				sourceEntry.filepath.c_str(),
				L"-E",
				wEntryPoint.c_str(),
				L"-HV",
				L"2021",
				L"-remove-unused-globals"
			};

			// #TODO_Ivar: Reenable once DXR has support for it.
			if (g_rhiCapabilities.supportsNative16BitOperations)
			{
				rewriteArgs.push_back(L"-enable-16bit-types");
			}

			RewriteResult rewriteResult = RewriteHLSL(rewriteArgs, sourceEntry.filepath, outProcessedSource);

			if (rewriteResult.succeded)
			{
				outProcessedSource = rewriteResult.outSource;
			}
			else
			{
				VT_LOGC_UNFORMATTED(Error, LogD3D12RHI, rewriteResult.error);
			}

			succeded = rewriteResult.succeded;
		}
#endif

		return succeded;
	}

	void D3D12ShaderCompiler::ReflectShader(const Specification& specification, CompilationResultData& inOutData, ID3D12ShaderReflection* reflectionData)
	{
		const ShaderStage currentShaderStage = specification.shaderSourceInfo.sourceEntry.shaderStage;

		ShaderParameterMap& shaderParameterMap = inOutData.shaderParameterMap;
		shaderParameterMap.SetShaderStage(currentShaderStage);

		D3D12_SHADER_DESC shaderDesc{};
		reflectionData->GetDesc(&shaderDesc);

		for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC shaderInputBindingDesc{};
			VT_D3D12_CHECK(reflectionData->GetResourceBindingDesc(i, &shaderInputBindingDesc));

			if (shaderInputBindingDesc.Type == D3D_SIT_CBUFFER)
			{
				shaderParameterMap.AddUniformBuffer(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);

				// If it's the globals uniform buffer we will extract the members
				// as they are the shaders parameters.
				if (std::string_view(shaderInputBindingDesc.Name) == "$Globals")
				{
					ID3D12ShaderReflectionConstantBuffer* reflectedCB = reflectionData->GetConstantBufferByIndex(i);
					D3D12_SHADER_BUFFER_DESC cbDesc{};
					reflectedCB->GetDesc(&cbDesc);

					for (uint32_t varIndex = 0; varIndex < cbDesc.Variables; ++varIndex)
					{
						ID3D12ShaderReflectionVariable* baseVariable = reflectedCB->GetVariableByIndex(varIndex);
						ID3D12ShaderReflectionType* baseType = baseVariable->GetType();
						D3D12_SHADER_TYPE_DESC baseTypeDesc;
						baseType->GetDesc(&baseTypeDesc);

						D3D12_SHADER_VARIABLE_DESC varDesc{};
						baseVariable->GetDesc(&varDesc);

						const ShaderUniformType type = Utility::GetShaderUniformTypeFromD3D12TypeDesc(baseTypeDesc);

						const uint32_t memberOffset = varDesc.StartOffset;
						const uint32_t memberSize = varDesc.Size;

						shaderParameterMap.AddParameter(varDesc.Name, type, memberSize, memberOffset);
					}
				}
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_STRUCTURED || shaderInputBindingDesc.Type == D3D_SIT_BYTEADDRESS)
			{
				shaderParameterMap.AddStructuredBufferSRV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_UAV_RWSTRUCTURED || shaderInputBindingDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS)
			{
				shaderParameterMap.AddStructuredBufferUAV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_TBUFFER)
			{
				shaderParameterMap.AddTexelBufferSRV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_UAV_RWTYPED)
			{
				if (shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE2D ||
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE2DARRAY || 
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE1D ||
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE1DARRAY ||
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURECUBE ||
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURECUBEARRAY ||
					shaderInputBindingDesc.Dimension == D3D_SRV_DIMENSION_TEXTURE3D)
				{
					shaderParameterMap.AddTextureUAV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
				}
				else
				{
					shaderParameterMap.AddTexelBufferUAV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
				}
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_TEXTURE)
			{
				shaderParameterMap.AddTextureSRV(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_SAMPLER)
			{
				shaderParameterMap.AddSampler(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
			else if (shaderInputBindingDesc.Type == D3D_SIT_RTACCELERATIONSTRUCTURE)
			{
				shaderParameterMap.AddAccelerationStructure(shaderInputBindingDesc.Name, shaderInputBindingDesc.Space, shaderInputBindingDesc.BindPoint, currentShaderStage);
			}
		}
	}

	D3D12ShaderCompiler::DxcCompilationResult D3D12ShaderCompiler::InvokeCompilerWithArguments(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source, HLSLIncluder* includer)
	{
		IDxcBlobEncoding* sourceBlob = nullptr;
		// Use first null character as size, as the string might contain many, which is invalid.
		size_t firstNullChar = source.find('\0');
		if (firstNullChar == std::string::npos)
		{
			firstNullChar = source.size();
		}

		m_hlslUtils->CreateBlob(source.c_str(), static_cast<uint32_t>(firstNullChar), CP_UTF8, &sourceBlob);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
		sourceBuffer.Size = sourceBlob->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* dxcCompilationOutput = nullptr;
		HRESULT hResult = m_hlslCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), includer, IID_PPV_ARGS(&dxcCompilationOutput));

		HRESULT hStatus;
		dxcCompilationOutput->GetStatus(&hStatus);

		const bool failed = FAILED(hResult) || FAILED(hStatus);

		DxcCompilationResult result{};
		result.dxcResult = dxcCompilationOutput;
		result.succeded = !failed;

		if (failed)
		{
			result.error = std::format("Failed to compile. Error: {}\n", hResult);
			result.error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(dxcCompilationOutput), sourceFilepath.string()));
		}
		else
		{
			// Get reflection data
			IDxcBlob* reflectionDataBlob = nullptr;
			hResult = dxcCompilationOutput->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionDataBlob), nullptr);

			if (SUCCEEDED(hResult))
			{
				DxcBuffer reflectionBuffer;
				reflectionBuffer.Ptr = reflectionDataBlob->GetBufferPointer();
				reflectionBuffer.Size = reflectionDataBlob->GetBufferSize();
				reflectionBuffer.Encoding = 0;

				hResult = m_hlslUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&result.reflectionData));
				if (FAILED(hResult))
				{
					VT_LOGC(Info, LogD3D12RHI, "Failed to create reflection data!");
				}
				
				reflectionDataBlob->Release();
			}
		}

		sourceBlob->Release();

		return result;
	}

	D3D12ShaderCompiler::RewriteResult D3D12ShaderCompiler::RewriteHLSL(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source)
	{
		IDxcBlobEncoding* sourceBlob = nullptr;
		// Use first null character as size, as the string might contain many, which is invalid.
		size_t firstNullChar = source.find('\0');
		if (firstNullChar == std::string::npos)
		{
			firstNullChar = source.size();
		}

		m_hlslUtils->CreateBlob(source.c_str(), static_cast<uint32_t>(firstNullChar), CP_UTF8, &sourceBlob);

		IDxcOperationResult* rewriteResult = nullptr;
		HRESULT hResult = m_hlslRewriter2->RewriteWithOptions(sourceBlob, sourceFilepath.c_str(), arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr, 0, nullptr, &rewriteResult);

		HRESULT hStatus;
		rewriteResult->GetStatus(&hStatus);

		const bool failed = FAILED(hResult) || FAILED(hStatus);

		RewriteResult result;
		result.succeded = !failed;

		if (failed)
		{
			result.error = std::format("Failed to rewrite. Error: {}\n", hResult);
			result.error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(rewriteResult), sourceFilepath.string()));
		}
		else
		{
			IDxcBlob* blob;
			rewriteResult->GetResult(&blob);

			result.outSource = std::string(reinterpret_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize());

			blob->Release();
		}

		sourceBlob->Release();
		rewriteResult->Release();

		return result;
	}
}
