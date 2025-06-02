#include "vkpch.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Shader/HLSLIncluder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

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

#include <dxsc/dxcapi.h>

#include <spirv_reflect.h>

#include <codecvt>
#include <locale>

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

		inline static ShaderUniformType GetShaderUniformTypeFromSpvTypeDesc(SpvReflectTypeDescription* typeDesc)
		{
			ShaderUniformType resultType;

			if (typeDesc->op == SpvOpTypeBool)
			{
				resultType.baseType = ShaderUniformBaseType::Bool;
			}
			else if (typeDesc->op == SpvOpTypeFloat)
			{
				if (typeDesc->traits.numeric.scalar.width == 16u)
				{
					resultType.baseType = ShaderUniformBaseType::Half;
				}
				else if (typeDesc->traits.numeric.scalar.width == 32u)
				{
					resultType.baseType = ShaderUniformBaseType::Float;
				}
				else if (typeDesc->traits.numeric.scalar.width == 64u)
				{
					resultType.baseType = ShaderUniformBaseType::Double;
				}
			}
			else if (typeDesc->op == SpvOpTypeInt)
			{
				if (typeDesc->traits.numeric.scalar.width == 16u)
				{
					resultType.baseType = typeDesc->traits.numeric.scalar.signedness ? ShaderUniformBaseType::Short : ShaderUniformBaseType::UShort;
				}
				else if (typeDesc->traits.numeric.scalar.width == 32u)
				{
					resultType.baseType = typeDesc->traits.numeric.scalar.signedness ? ShaderUniformBaseType::Int : ShaderUniformBaseType::UInt;
				}
				else if (typeDesc->traits.numeric.scalar.width == 64u)
				{
					resultType.baseType = typeDesc->traits.numeric.scalar.signedness ? ShaderUniformBaseType::Int64 : ShaderUniformBaseType::UInt64;
				}
			}

			resultType.vecsize = typeDesc->traits.numeric.vector.component_count;

			return resultType;
		}
	}

	VulkanShaderCompiler::VulkanShaderCompiler(const ShaderCompilerCreateInfo& createInfo)
		: m_includeDirectories(createInfo.includeDirectories), m_macros(createInfo.initialMacros), m_flags(createInfo.flags),
		m_shaderCache(createInfo.shaderCache)
	{
		VT_LOGC(Trace, LogVulkanRHI, "Initializing VulkanShaderCompiler");
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_hlslCompiler));
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_hlslUtils));
	}

	VulkanShaderCompiler::~VulkanShaderCompiler()
	{
		m_hlslUtils->Release();
		m_hlslCompiler->Release();

		VT_LOGC(Trace, LogVulkanRHI, "Destroying VulkanShaderCompiler");
	}

	void VulkanShaderCompiler::AddMacroImpl(const std::string& macroName)
	{
		if (std::find(m_macros.begin(), m_macros.end(), macroName) != m_macros.end())
		{
			return;
		}

		m_macros.push_back(macroName);
	}

	void VulkanShaderCompiler::RemoveMacroImpl(std::string_view macroName)
	{
		if (auto it = std::find(m_macros.begin(), m_macros.end(), macroName); it != m_macros.end())
		{
			m_macros.erase(it);
		}
	}

	void* VulkanShaderCompiler::GetHandleImpl() const
	{
		return nullptr;
	}

	ShaderCompiler::CompilationResultData VulkanShaderCompiler::TryCompileImpl(const Specification& specification)
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
			VT_LOGC(Error, LogVulkanRHI, "Trying to compile a shader without a source!");
			return {};
		}

		CompilationResultData result = CompileShader(specification);
		if (result.result != ShaderCompiler::CompilationResult::Success)
		{
			const auto cachedResult = m_shaderCache->TryGetCachedShader(specification);
			return cachedResult.data;
		}

		ReflectShader(specification, result);
		m_shaderCache->CacheShader(specification, result);

		return result;
	}

	ShaderCompiler::CompilationResultData VulkanShaderCompiler::CompileShader(const Specification& specification)
	{
		CompilationResultData result;

		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;
		std::string processedSource = specification.shaderSourceInfo.source;

		if (!PreprocessSource(specification, processedSource))
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
			L"-spirv",
			L"-fspv-target-env=vulkan1.3",
			L"-HV",
			L"2021",
			L"-D", L"__VULKAN__ ",
			L"-fvk-use-dx-layout",
			L"-fvk-bind-globals", globalsBinding.c_str(), globalsSpace.c_str(),

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
			//arguments.push_back(L"-fspv-debug=vulkan");
		}

		const ShaderStage shaderStage = sourceEntry.shaderStage;
		if (shaderStage == ShaderStage::Vertex || shaderStage == ShaderStage::Hull || shaderStage == ShaderStage::Geometry)
		{
			arguments.emplace_back(L"-fvk-invert-y");
		}

		// Custom pre processing
		{
			PreProcessorData processingData{};
			processingData.shaderSource = processedSource;
			processingData.shaderStage = shaderStage;
			processingData.entryPoint = sourceEntry.entryPoint;

			PreProcessorResult preProcessorResult{};
			if (!ShaderPreProcessor::PreProcessShaderSource(processingData, preProcessorResult))
			{
				result.result = CompilationResult::PreprocessFailed;
				return result;
			}

			if (shaderStage == ShaderStage::Pixel)
			{
				result.outputFormats = preProcessorResult.outputFormats;
			}
			else if (shaderStage == ShaderStage::Vertex)
			{
				result.vertexLayout = preProcessorResult.vertexLayout;
				result.instanceLayout = preProcessorResult.instanceLayout;
			}

			processedSource = preProcessorResult.preProcessedResult;
		}

		// Compile
		IDxcBlobEncoding* sourcePtr = nullptr;
		m_hlslUtils->CreateBlob(processedSource.c_str(), static_cast<uint32_t>(processedSource.size()), CP_UTF8, &sourcePtr);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourcePtr->GetBufferPointer();
		sourceBuffer.Size = sourcePtr->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* compiledBinary = nullptr;
		std::string error;

		HRESULT compilationResult = m_hlslCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr, IID_PPV_ARGS(&compiledBinary));

		const bool failed = FAILED(compilationResult);
		if (failed)
		{
			error = std::format("Failed to compile. Error: {}\n", compilationResult);
			error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compiledBinary), sourceEntry.filepath.string()));
		}

		if (error.empty())
		{
			IDxcBlob* shaderResult = nullptr;
			compiledBinary->GetResult(&shaderResult);

			if (!shaderResult || shaderResult->GetBufferSize() == 0)
			{
				error = std::format("Failed to compile. Error: {}\n", compilationResult);
				error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compiledBinary), sourceEntry.filepath.string()));

				VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, error);

				sourcePtr->Release();
				compiledBinary->Release();

				result.result = CompilationResult::Failure;
				return result;
			}

			const size_t size = shaderResult->GetBufferSize();

			result.shaderBinary.resize_uninitialized(size / sizeof(uint32_t));
			memcpy_s(result.shaderBinary.data(), size, shaderResult->GetBufferPointer(), shaderResult->GetBufferSize());

			shaderResult->Release();
			result.result = ShaderCompiler::CompilationResult::Success;
		}
		else
		{
			sourcePtr->Release();
			compiledBinary->Release();

			VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, error);
			result.result = ShaderCompiler::CompilationResult::Failure;
			return result;
		}

		sourcePtr->Release();
		compiledBinary->Release();

		VT_LOGC(Info, LogVulkanRHI, "Successfully compiled shader {}!", sourceEntry.filepath);
		return result;
	}

	bool VulkanShaderCompiler::PreprocessSource(const Specification& specification, std::string& outProcessedSource)
	{
		Vector<std::wstring> wIncludeDirs;
		Vector<const wchar_t*> wcIncludeDirs;

		for (const auto& includeDir : m_includeDirectories)
		{
			wIncludeDirs.push_back(L"-I " + includeDir.wstring());
		}

		for (const auto& includeDir : wIncludeDirs)
		{
			wcIncludeDirs.push_back(includeDir.c_str());
		}

		Vector<const wchar_t*> arguments =
		{
			specification.shaderSourceInfo.sourceEntry.filepath.c_str(),
			L"-P", // Preprocess
			L"-D", L"__HLSL__",
			L"-D", L"__VULKAN__"
		};

		// Append permutations
		Vector<std::wstring> permutationStrings = specification.permutationConfig.GetPermutationsWideStr();
		for (const auto& permutationStr : permutationStrings)
		{
			arguments.push_back(L"-D");
			arguments.push_back(permutationStr.c_str());
		}

		// Append include dirs
		for (const auto& includeDir : wcIncludeDirs)
		{
			arguments.push_back(includeDir);
		}

		// Append global macros
		Vector<std::wstring> wMacros;
		for (const auto& macro : m_macros)
		{
			wMacros.push_back(::Utility::ToWString(macro));
		}

		if ((m_flags & ShaderCompilerFlags::EnableShaderValidator) != ShaderCompilerFlags::None)
		{
			wMacros.push_back(L"ENABLE_RUNTIME_VALIDATION");
		}

		for (const auto& macro : wMacros)
		{
			arguments.emplace_back(L"-D");
			arguments.emplace_back(macro.c_str());
		}

		// Append compile flags
		if ((m_flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		IDxcBlobEncoding* sourcePtr = nullptr;
		m_hlslUtils->CreateBlob(outProcessedSource.c_str(), static_cast<uint32_t>(outProcessedSource.size()), CP_UTF8, &sourcePtr);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourcePtr->GetBufferPointer();
		sourceBuffer.Size = sourcePtr->GetBufferSize();
		sourceBuffer.Encoding = 0;

		const Scope<HLSLIncluder> includer = CreateScope<HLSLIncluder>();

		IDxcResult* compilationResult = nullptr;
		HRESULT result = m_hlslCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), includer.get(), IID_PPV_ARGS(&compilationResult));

		std::string error;
		const bool failed = FAILED(result);

		if (failed)
		{
			error = std::format("Failed to compile. Error {0}\n", result);
			error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compilationResult), specification.shaderSourceInfo.sourceEntry.filepath.string()));
		}

		if (error.empty())
		{
			IDxcBlob* compileResult = nullptr;
			compilationResult->GetResult(&compileResult);

			outProcessedSource = reinterpret_cast<const char*>(compileResult->GetBufferPointer());
			compileResult->Release();
		}
		else
		{
			VT_LOGC(Error, LogVulkanRHI, error);
		}

		sourcePtr->Release();
		compilationResult->Release();

		return !failed;
	}

	void VulkanShaderCompiler::ReflectShader(const Specification& specification, CompilationResultData& inOutData)
	{
		SpvReflectShaderModule spirvModule{};
		SpvReflectResult result = spvReflectCreateShaderModule(inOutData.shaderBinary.size() * sizeof(uint32_t), inOutData.shaderBinary.data(), &spirvModule);
		VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

		uint32_t count;
		result = spvReflectEnumerateDescriptorSets(&spirvModule, &count, nullptr);
		VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

		Vector<SpvReflectDescriptorSet*> sets(count);
		result = spvReflectEnumerateDescriptorSets(&spirvModule, &count, sets.data());
		VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

		Vector<SpvReflectDescriptorBinding*> uniformBuffers;
		Vector<SpvReflectDescriptorBinding*> storageBuffers;
		Vector<SpvReflectDescriptorBinding*> uniformTexelBuffers;
		Vector<SpvReflectDescriptorBinding*> storageTexelBuffers;
		Vector<SpvReflectDescriptorBinding*> storageImages;
		Vector<SpvReflectDescriptorBinding*> images;
		Vector<SpvReflectDescriptorBinding*> samplers;

		for (size_t i = 0; i < sets.size(); ++i)
		{
			const SpvReflectDescriptorSet* spvSet = sets[0];

			for (uint32_t binding = 0; binding < spvSet->binding_count; ++binding)
			{
				SpvReflectDescriptorBinding* spvBinding = spvSet->bindings[binding];

				if (spvBinding->accessed)
				{
					switch (spvBinding->resource_type)
					{
						case SPV_REFLECT_RESOURCE_FLAG_CBV: uniformBuffers.emplace_back(spvBinding); break;
						case SPV_REFLECT_RESOURCE_FLAG_SAMPLER: samplers.emplace_back(spvBinding); break;
						case SPV_REFLECT_RESOURCE_FLAG_SRV:
						{
							switch (spvBinding->descriptor_type)
							{
								case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: images.emplace_back(spvBinding); break;
								case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: storageBuffers.emplace_back(spvBinding); break;
								case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: uniformTexelBuffers.emplace_back(spvBinding); break;
							}
							break;
						}

						case SPV_REFLECT_RESOURCE_FLAG_UAV:
						{
							switch (spvBinding->descriptor_type)
							{
								case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: storageImages.emplace_back(spvBinding); break;
								case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: storageBuffers.emplace_back(spvBinding); break;
								case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: storageTexelBuffers.emplace_back(spvBinding); break;;
							}
							break;
						}
					}
				}
			}
		}

		Vector<SpvReflectDescriptorBinding*> allBindings;
		allBindings.append(uniformBuffers);
		allBindings.append(storageBuffers);
		allBindings.append(uniformTexelBuffers);
		allBindings.append(storageTexelBuffers);
		allBindings.append(storageImages);
		allBindings.append(images);
		allBindings.append(samplers);

		// Change all descriptor set indices to be the same
		// Because we always add uniform buffers first, the globals UB will always end up at binding index 0.
		const ShaderStage currentShaderStage = specification.shaderSourceInfo.sourceEntry.shaderStage;
		const uint32_t shaderStageDescriptorSetIndex = GetDescriptorSetIndexFromShaderStage(currentShaderStage);
		for (uint32_t bindingIndex = 0; SpvReflectDescriptorBinding* binding : allBindings)
		{
			result = spvReflectChangeDescriptorBindingNumbers(&spirvModule, binding, bindingIndex, shaderStageDescriptorSetIndex);
			VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			bindingIndex++;
		}

		ShaderParameterMap& shaderParameterMap = inOutData.shaderParameterMap;
		shaderParameterMap.SetShaderStage(currentShaderStage);

		for (SpvReflectDescriptorBinding* uniformBuffer : uniformBuffers)
		{
			shaderParameterMap.AddUniformBuffer(uniformBuffer->name, uniformBuffer->set, uniformBuffer->binding, currentShaderStage);

			// If it's the globals uniform buffer we will extract the members
			// as they are the shaders parameters.
			if (std::string_view(uniformBuffer->name) == "$Globals")
			{
				for (uint32_t memberIndex = 0; memberIndex < uniformBuffer->block.member_count; memberIndex++)
				{
					const SpvReflectBlockVariable& member = uniformBuffer->block.members[memberIndex];
					const ShaderUniformType uniformType = Utility::GetShaderUniformTypeFromSpvTypeDesc(member.type_description);

					shaderParameterMap.AddParameter(member.name, uniformType, member.size, member.absolute_offset);
				}
			}
		}

		for (SpvReflectDescriptorBinding* storageBuffer : storageBuffers)
		{
			shaderParameterMap.AddStructuredBufferSRV(storageBuffer->name, storageBuffer->set, storageBuffer->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* uniformTexelBuffer : uniformTexelBuffers)
		{
			shaderParameterMap.AddTexelBufferSRV(uniformTexelBuffer->name, uniformTexelBuffer->set, uniformTexelBuffer->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* storageTexelBuffer : storageTexelBuffers)
		{
			shaderParameterMap.AddTexelBufferUAV(storageTexelBuffer->name, storageTexelBuffer->set, storageTexelBuffer->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* storageImage : storageImages)
		{
			shaderParameterMap.AddTextureUAV(storageImage->name, storageImage->set, storageImage->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* image : images)
		{
			shaderParameterMap.AddTextureSRV(image->name, image->set, image->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* sampler : samplers)
		{
			shaderParameterMap.AddSampler(sampler->name, sampler->set, sampler->binding, currentShaderStage);
		}

		const uint32_t spirvSize = spvReflectGetCodeSize(&spirvModule);
		inOutData.shaderBinary.resize(spirvSize / sizeof(uint32_t));
		memcpy(inOutData.shaderBinary.data(), spvReflectGetCode(&spirvModule), spirvSize);
	}
}
