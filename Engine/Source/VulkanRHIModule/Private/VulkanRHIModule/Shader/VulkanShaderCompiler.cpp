#include "vkpch.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Shader/HLSLIncluder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/RayTracing/RayTracingTableDescriptorSetManager.h"

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
#include <dxsc/dxctools.h>

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

		inline static const std::string GetErrorStringFromResult(IDxcOperationResult* result)
		{
			IDxcBlobEncoding* error;
			result->GetErrorBuffer(&error);

			return std::string(reinterpret_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
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
		DxcCreateInstance(CLSID_DxcRewriter, IID_PPV_ARGS(&m_hlslRewriter));

		m_hlslRewriter->QueryInterface(&m_hlslRewriter2);
	}

	VulkanShaderCompiler::~VulkanShaderCompiler()
	{
		m_hlslUtils->Release();
		m_hlslCompiler->Release();
		m_hlslRewriter->Release();
		m_hlslRewriter2->Release();

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

			VT_LOGC(Info, LogVulkanRHI, "Successfully compiled shader {}!", sourceEntry.filepath);
		}
		else
		{
			VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, compilationResult.error);
			result.result = ShaderCompiler::CompilationResult::Failure;
		}

		if (compilationResult.dxcResult)
		{
			compilationResult.dxcResult->Release();
		}

		return result;
	}

	bool VulkanShaderCompiler::PreprocessSource(const Specification& specification, std::string& outProcessedSource, CompilationResultData& compilationResult)
	{
		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;

		Vector<std::wstring> wIncludeDirs;
		Vector<const wchar_t*> wcIncludeDirs;

		// Add platform include
		constexpr std::string_view platformInclude = "#include \"Platforms/Vulkan/VulkanInterop.hlsli\"\n";
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
			L"-D", L"__VULKAN__"
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
				VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, preProcessingResult.error);
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
				VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, rewriteResult.error);
			}

			succeded = rewriteResult.succeded;
		}
#endif

		return succeded;
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
		Vector<SpvReflectDescriptorBinding*> accelerationStructures;

		for (size_t i = 0; i < sets.size(); ++i)
		{
			const SpvReflectDescriptorSet* spvSet = sets[i];

			// First find the globals UB, to make sure that it always gets binding 0
			for (uint32_t binding = 0; binding < spvSet->binding_count; ++binding)
			{
				SpvReflectDescriptorBinding* spvBinding = spvSet->bindings[binding];
				if (strcmp(spvBinding->name, "$Globals") == 0)
				{
					uniformBuffers.emplace_back(spvBinding);
					break;
				}
			}

			for (uint32_t binding = 0; binding < spvSet->binding_count; ++binding)
			{
				SpvReflectDescriptorBinding* spvBinding = spvSet->bindings[binding];

				if (spvBinding->accessed && strcmp(spvBinding->name, "$Globals") != 0)
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
								case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: accelerationStructures.emplace_back(spvBinding); break;
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
		allBindings.append(accelerationStructures);

		// Change all descriptor set indices to be the same
		// Because we always add uniform buffers first, the globals UB will always end up at binding index 0.
		const ShaderStage currentShaderStage = specification.shaderSourceInfo.sourceEntry.shaderStage;
		const uint32_t shaderStageDescriptorSetIndex = GetDescriptorSetIndexFromShaderStage(currentShaderStage);
		for (uint32_t bindingIndex = 0; SpvReflectDescriptorBinding* binding : allBindings)
		{
			if (binding->set == RayTracingTableDescriptorSetManager::Set && (binding->binding == RayTracingTableDescriptorSetManager::BuffersBinding || binding->binding == RayTracingTableDescriptorSetManager::TexturesBinding))
			{
				continue;
			}

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
			// Special case for ray tracing resource table
			if (storageBuffer->set == RayTracingTableDescriptorSetManager::Set && storageBuffer->binding == RayTracingTableDescriptorSetManager::BuffersBinding)
			{
				shaderParameterMap.SetAccessesRayTracingResourceTable();
			}
			else
			{
				shaderParameterMap.AddStructuredBufferSRV(storageBuffer->name, storageBuffer->set, storageBuffer->binding, currentShaderStage);
			}
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
			// Special case for ray tracing resource table
			if (image->set == RayTracingTableDescriptorSetManager::Set && image->binding == RayTracingTableDescriptorSetManager::TexturesBinding)
			{
				shaderParameterMap.SetAccessesRayTracingResourceTable();
			}
			else
			{
				shaderParameterMap.AddTextureSRV(image->name, image->set, image->binding, currentShaderStage);
			}
		}

		for (SpvReflectDescriptorBinding* sampler : samplers)
		{
			shaderParameterMap.AddSampler(sampler->name, sampler->set, sampler->binding, currentShaderStage);
		}

		for (SpvReflectDescriptorBinding* accelerationStructure : accelerationStructures)
		{
			shaderParameterMap.AddAccelerationStructure(accelerationStructure->name, accelerationStructure->set, accelerationStructure->binding, currentShaderStage);
		}

		const uint32_t spirvSize = spvReflectGetCodeSize(&spirvModule);
		inOutData.shaderBinary.resize(spirvSize / sizeof(uint32_t));
		memcpy(inOutData.shaderBinary.data(), spvReflectGetCode(&spirvModule), spirvSize);

		spvReflectDestroyShaderModule(&spirvModule);
	}

	VulkanShaderCompiler::DxcCompilationResult VulkanShaderCompiler::InvokeCompilerWithArguments(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source, HLSLIncluder* includer)
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

		sourceBlob->Release();
	
		return result;
	}

  	VulkanShaderCompiler::RewriteResult VulkanShaderCompiler::RewriteHLSL(Vector<const wchar_t*>& arguments, const std::filesystem::path& sourceFilepath, const std::string& source)
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
