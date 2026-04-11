#include "vkpch.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Shader/HLSLIncluder.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Descriptors/ResourceTableDescriptorSetManager.h"
#include "VulkanRHIModule/Pipelines/StaticSamplerDescriptorSetManager.h"

#include <CoreModule/Project/ProjectManager.h>
#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/FileIORequest.h>
#include <FileSystemModule/IOThreads/IOThreads.h>

#include <RHIModule/Shader/ShaderUtility.h>
#include <RHIModule/Shader/ShaderPreProcessor.h>
#include <RHIModule/Shader/ShaderCache.h>
#include <RHIModule/Globals.h>
#include <RHIModule/RHICapabilities.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/ThreadConfig.h>

#ifdef _WIN32
#include <wrl.h>
#else
#include <dxc/WinAdapter.h>
#endif

#include <dxc/dxcapi.h>
#include <dxc/dxctools.h>

#include <spirv_reflect.h>
#include <spirv-tools/optimizer.hpp>

namespace Volt::RHI
{
	namespace Utility
	{
		inline static const String GetErrorStringFromResult(IDxcResult* result)
		{
			String output;

			IDxcBlobUtf8* errors;
			result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
			if (errors && errors->GetStringLength() > 0)
			{
				output = (char*)errors->GetBufferPointer();
				errors->Release();
			}

			return output;
		}

		inline static const String GetErrorStringFromResult(IDxcOperationResult* result)
		{
			IDxcBlobEncoding* error;
			result->GetErrorBuffer(&error);

			return String(reinterpret_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
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
		: m_createInfo(createInfo),
		m_includeDirectories(createInfo.includeDirectories),
		m_macros(createInfo.initialMacros),
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

	void VulkanShaderCompiler::AddMacroImpl(const String& macroName)
	{
		if (std::find(m_macros.begin(), m_macros.end(), macroName) != m_macros.end())
		{
			return;
		}

		m_macros.push_back(macroName);
	}

	void VulkanShaderCompiler::RemoveMacroImpl(StringView macroName)
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
		VT_PROFILE_FUNCTION();

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
		
		if (ShouldDumpShaderDebugInfo())
		{
			DumpSpirv(specification, result);
		}
		
		m_shaderCache->CacheShader(specification, result);

		return result;
	}

	ShaderCompiler::CompilationResultData VulkanShaderCompiler::CompileShader(const Specification& specification)
	{
		VT_PROFILE_FUNCTION();

		CompilationResultData result;

		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;
		String processedSource = specification.shaderSourceInfo.source;

		if (!PreprocessSource(specification, processedSource, result))
		{
			result.result = ShaderCompiler::CompilationResult::PreprocessFailed;
			return result;
		}

		const WString wEntryPoint(WString::CtorConvert(), sourceEntry.entryPoint);
		const WString globalsBinding = FormatString(L"{}", Globals::SHADER_GLOBALS_BINDING);
		const WString globalsSpace = FormatString(L"{}", Globals::SHADER_GLOBALS_SPACE);

		Vector<const wchar_t*> arguments =
		{
			sourceEntry.filepath.CStr(),
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

		if (ShouldDumpShaderDebugInfo())
		{
			DumpShaderText(specification, processedSource);
		}

		// Append permutations
		const Vector<WString> permutationStrings = specification.permutationConfig.GetPermutationsWideStr();
		for (const auto& permutationStr : permutationStrings)
		{
			arguments.push_back(L"-D");
			arguments.push_back(permutationStr.c_str());
		}

		if (g_rhiCapabilities.supportsNative16BitOperations)
		{
			arguments.push_back(L"-enable-16bit-types");
		}

		if ((m_createInfo.flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		ShaderOptimizationLevel optimizationLevel = m_createInfo.optimizationLevel;

		// If we want shader debug into, switch the optimization level to debug
		if ((m_createInfo.flags & ShaderCompilerFlags::OutputShaderDebugInfo) != ShaderCompilerFlags::None)
		{
			optimizationLevel = ShaderOptimizationLevel::Disable;
		}

		switch (optimizationLevel)
		{
			case ShaderOptimizationLevel::Disable: arguments.push_back(L"-Od"); break;
			case ShaderOptimizationLevel::Release: arguments.push_back(L"-O1"); break;
			case ShaderOptimizationLevel::Dist: arguments.push_back(L"-O3"); break;
		}

		if (optimizationLevel == ShaderOptimizationLevel::Disable)
		{
			arguments.push_back(DXC_ARG_DEBUG);
			//arguments.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
			//arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);
			//arguments.push_back(L"-Qembed_debug");
			arguments.push_back(L"-fspv-debug=vulkan-with-source");
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

	bool VulkanShaderCompiler::PreprocessSource(const Specification& specification, String& outProcessedSource, CompilationResultData& compilationResult)
	{
		VT_PROFILE_FUNCTION();

		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;

		Vector<WString> wIncludeDirs;
		Vector<const wchar_t*> wcIncludeDirs;

		// Add platform include
		constexpr StringView platformInclude = "#include \"Platforms/Vulkan/VulkanInterop.hlsli\"\n";
		outProcessedSource.insert(outProcessedSource.begin(), platformInclude.begin(), platformInclude.end());

		for (const auto& includeDir : m_includeDirectories)
		{
			wIncludeDirs.push_back(L"-I " + includeDir.ToWString());
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
		Vector<WString> permutationStrings = specification.permutationConfig.GetPermutationsWideStr();
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
		Vector<WString> wMacros;
		for (const auto& macro : m_macros)
		{
			wMacros.emplace_back(WString::CtorConvert(), macro);
		}

		for (const auto& macro : wMacros)
		{
			definesAndIncludes.emplace_back(L"-D");
			definesAndIncludes.emplace_back(macro.c_str());
		}

		// Append compile flags
		if ((m_createInfo.flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			definesAndIncludes.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		bool succeded = false;

		// Pre process source
		{
			Vector<const wchar_t*> compilationArgs =
			{
				sourceEntry.filepath.CStr(),
				L"-P", // Preprocess
			};
			compilationArgs.append(definesAndIncludes);

			const Unique<HLSLIncluder> includer = CreateUnique<HLSLIncluder>();
			DxcCompilationResult preProcessingResult = InvokeCompilerWithArguments(compilationArgs, sourceEntry.filepath, outProcessedSource, includer.GetRaw());

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

		return succeded;
	}

	void VulkanShaderCompiler::ReflectShader(const Specification& specification, CompilationResultData& inOutData)
	{
		VT_PROFILE_FUNCTION();

		Vector<uint32_t> optimizedSpirv;
		OptimizeSpirvForReflection(specification, inOutData, optimizedSpirv);
		inOutData.shaderBinary = std::move(optimizedSpirv);

		ReflectAndRewriteSpirv(specification.shaderSourceInfo.sourceEntry.shaderStage, inOutData.shaderBinary, inOutData.shaderParameterMap);
	}

	VulkanShaderCompiler::DxcCompilationResult VulkanShaderCompiler::InvokeCompilerWithArguments(Vector<const wchar_t*>& arguments, const Filesystem::Path& sourceFilepath, const String& source, HLSLIncluder* includer)
	{
		VT_PROFILE_FUNCTION();

		IDxcBlobEncoding* sourceBlob = nullptr;
		// Use first null character as size, as the string might contain many, which is invalid.
		size_t firstNullChar = source.find('\0');
		if (firstNullChar == String::npos)
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
			result.error = FormatString("Failed to compile. Error: {}\n", hResult);
			result.error.append(FormatString("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(dxcCompilationOutput), sourceFilepath.ToString()));
		}

		sourceBlob->Release();
	
		return result;
	}

	void VulkanShaderCompiler::OptimizeSpirvForReflection(const Specification& specification, CompilationResultData& inOutData, Vector<uint32_t>& outSpirv)
	{
		spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_3);
		tools.SetMessageConsumer([](spv_message_level_t messageLevel, const char* source, const spv_position_t& position, const char* message)
		{
			VT_LOG(Error, "{}", message);
		});


		bool result = tools.Validate(inOutData.shaderBinary.data(), inOutData.shaderBinary.size());

		VT_ENSURE(result);

		spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_3);
		optimizer.SetMessageConsumer([](spv_message_level_t messageLevel, const char* source, const spv_position_t& position, const char* message) 
		{
			VT_LOG(Error, "{}", message);
		});

		optimizer.RegisterPass(spvtools::CreateDeadVariableEliminationPass());
		optimizer.RegisterPass(spvtools::CreateEliminateDeadConstantPass());
		optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());
		optimizer.RegisterPass(spvtools::CreateEliminateDeadInputComponentsSafePass());
		optimizer.RegisterPass(spvtools::CreateEliminateDeadMembersPass());
		optimizer.RegisterPass(spvtools::CreateEliminateDeadOutputComponentsPass());
		optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());

		std::vector<uint32_t> optimized;
		result = optimizer.Run(inOutData.shaderBinary.data(), inOutData.shaderBinary.size(), &optimized);

		VT_ENSURE(result);

		result = tools.Validate(optimized);

		VT_ENSURE(result);

		outSpirv.resize_uninitialized(optimized.size());
		memcpy_s(outSpirv.data(), outSpirv.byte_size(), optimized.data(), optimized.size() * sizeof(uint32_t));
	}

	void VulkanShaderCompiler::ReflectAndRewriteSpirv(ShaderStage currentShaderStage, Vector<uint32_t>& spirv, ShaderParameterMap& shaderParameterMap)
	{
		VT_PROFILE_FUNCTION();

		SpvReflectShaderModule spirvModule{};
		SpvReflectResult result = spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &spirvModule);
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
		const uint32_t shaderStageDescriptorSetIndex = GetDescriptorSetIndexFromShaderStage(currentShaderStage);
		for (uint32_t bindingIndex = 0; SpvReflectDescriptorBinding* binding : allBindings)
		{
			if (binding->set == ResourceTableDescriptorSetManager::Set && (binding->binding == ResourceTableDescriptorSetManager::BuffersBinding || binding->binding == ResourceTableDescriptorSetManager::TexturesBinding))
			{
				continue;
			}

			if (binding->set == StaticSamplerDescriptorSetManager::Set)
			{
				continue;
			}

			result = spvReflectChangeDescriptorBindingNumbers(&spirvModule, binding, bindingIndex, shaderStageDescriptorSetIndex);
			VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			bindingIndex++;
		}

		shaderParameterMap.SetShaderStage(currentShaderStage);

		for (SpvReflectDescriptorBinding* uniformBuffer : uniformBuffers)
		{
			shaderParameterMap.AddUniformBuffer(uniformBuffer->name, uniformBuffer->set, uniformBuffer->binding, currentShaderStage);

			// If it's the globals uniform buffer we will extract the members
			// as they are the shaders parameters.
			if (StringView(uniformBuffer->name) == "$Globals")
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
			if (storageBuffer->set == ResourceTableDescriptorSetManager::Set && storageBuffer->binding == ResourceTableDescriptorSetManager::BuffersBinding)
			{
				shaderParameterMap.SetAccessesResourceTable();
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
			if (image->set == ResourceTableDescriptorSetManager::Set && image->binding == ResourceTableDescriptorSetManager::TexturesBinding)
			{
				shaderParameterMap.SetAccessesResourceTable();
			}
			else
			{
				shaderParameterMap.AddTextureSRV(image->name, image->set, image->binding, currentShaderStage);
			}
		}

		for (SpvReflectDescriptorBinding* sampler : samplers)
		{
			// Make sure static samplers aren't included.
			if (sampler->set != StaticSamplerDescriptorSetManager::Set)
			{
				shaderParameterMap.AddSampler(sampler->name, sampler->set, sampler->binding, currentShaderStage);
			}
		}

		for (SpvReflectDescriptorBinding* accelerationStructure : accelerationStructures)
		{
			shaderParameterMap.AddAccelerationStructure(accelerationStructure->name, accelerationStructure->set, accelerationStructure->binding, currentShaderStage);
		}

		// "InlineParameterBlock"
		{
			result = spvReflectEnumeratePushConstantBlocks(&spirvModule, &count, nullptr);
			VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			Vector<SpvReflectBlockVariable*> pushConstants(count);
			result = spvReflectEnumeratePushConstantBlocks(&spirvModule, &count, pushConstants.data());
			VT_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);

			for (const SpvReflectBlockVariable* var : pushConstants)
			{
				const ShaderUniformType uniformType = Utility::GetShaderUniformTypeFromSpvTypeDesc(var->type_description);
				shaderParameterMap.AddInlineParameter(var->name, uniformType, var->size, var->absolute_offset);
			}
		}

		const uint32_t spirvSize = spvReflectGetCodeSize(&spirvModule);
		spirv.resize(spirvSize / sizeof(uint32_t));
		memcpy(spirv.data(), spvReflectGetCode(&spirvModule), spirvSize);

		spvReflectDestroyShaderModule(&spirvModule);
	}

	void VulkanShaderCompiler::DumpSpirv(const Specification& specification, const CompilationResultData& data)
	{
		const Filesystem::Path dumpDirectory = GetShaderDumpDirectory(specification);

		if (!Filesystem::Exists(dumpDirectory))
		{
			Filesystem::CreateDirectories(dumpDirectory);
		}

		const Filesystem::Path filepath = dumpDirectory / specification.shaderSourceInfo.sourceEntry.filepath.Stem() + ".spv";

		// We only need to copy the data if we are not on an IO thread.
		const bool createCopyOfData = !Threads::GetThreadConfig().isIOThread;
		IORequestResult<IORequestWriteFile_Binary> ioResult = IOThreads::SubmitRequest<IORequestWriteFile_Binary>("Write Shader Dump (Text)", filepath, data.shaderBinary.data(), data.shaderBinary.byte_size(), createCopyOfData);
	}
	
	void VulkanShaderCompiler::DumpShaderText(const Specification& specification, StringView shaderText)
	{
		const Filesystem::Path dumpDirectory = GetShaderDumpDirectory(specification);

		if (!Filesystem::Exists(dumpDirectory))
		{
			Filesystem::CreateDirectories(dumpDirectory);
		}

		const Filesystem::Path filepath = dumpDirectory / specification.shaderSourceInfo.sourceEntry.filepath.Filename();
		IORequestResult<IORequestWriteFile_String> ioResult = IOThreads::SubmitRequest<IORequestWriteFile_String>("Write Shader Dump (Text)", filepath, shaderText);
	}

	bool VulkanShaderCompiler::ShouldDumpShaderDebugInfo() const
	{
		return (m_createInfo.flags & ShaderCompilerFlags::OutputShaderDebugInfo) != ShaderCompilerFlags::None;
	}

	Filesystem::Path VulkanShaderCompiler::GetShaderDumpDirectory(const Specification& specification) const
	{
		const Filesystem::Path& filepath = specification.shaderSourceInfo.sourceEntry.filepath;
		const Filesystem::Path shaderDirectory = filepath.Stem() / FormatString("{}", specification.permutationConfig.GetPermutationIndex());

		return ProjectManager::GetGeneratedDirectory() / "ShaderDebugInfo" / "Vulkan" / shaderDirectory;
	}
}
