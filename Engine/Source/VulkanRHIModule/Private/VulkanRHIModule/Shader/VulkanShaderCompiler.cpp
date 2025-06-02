#include "vkpch.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Shader/VulkanShader.h"
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

#include <spirv_cross/spirv_glsl.hpp>
#include <spirv-tools/libspirv.h>

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

		inline static const ShaderUniformType GetShaderUniformTypeFromSPIRV(const spirv_cross::SPIRType& type)
		{
			ShaderUniformType resultType;

			switch (type.basetype)
			{
				case spirv_cross::SPIRType::Boolean: resultType.baseType = ShaderUniformBaseType::Bool; break;
				case spirv_cross::SPIRType::UInt: resultType.baseType = ShaderUniformBaseType::UInt; break;
				case spirv_cross::SPIRType::Int: resultType.baseType = ShaderUniformBaseType::Int; break;
				case spirv_cross::SPIRType::Float: resultType.baseType = ShaderUniformBaseType::Float; break;
				case spirv_cross::SPIRType::Half: resultType.baseType = ShaderUniformBaseType::Half; break;
				case spirv_cross::SPIRType::Short: resultType.baseType = ShaderUniformBaseType::Short; break;
				case spirv_cross::SPIRType::UShort: resultType.baseType = ShaderUniformBaseType::UShort; break;
			}

			resultType.columns = type.columns;
			resultType.vecsize = type.vecsize;

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

	ShaderCompiler::CompilationResultData VulkanShaderCompiler::TryCompileImpl(const Specification& specification)
	{
		// First try and get cached shader
		if (!specification.forceCompile)
		{
			const auto cachedResult = m_shaderCache->TryGetCachedShader(specification);
			if (cachedResult.data.IsValid())
			{
				return cachedResult.data;
			}
		}

		if (specification.shaderSourceInfo.empty())
		{
			VT_LOGC(Error, LogVulkanRHI, "Trying to compile a shader without sources!");
			return {};
		}

		CompilationResultData result = CompileAll(specification);

		// If compilation fails, we try to get the cached version.
		if (result.result != ShaderCompiler::CompilationResult::Success)
		{
			const auto cachedResult = m_shaderCache->TryGetCachedShader(specification);
			return cachedResult.data;
		}

		ReflectAllStages(specification, result);
		m_shaderCache->CacheShader(specification, result);

		return result;
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

	ShaderCompiler::CompilationResultData VulkanShaderCompiler::CompileAll(const Specification& specification)
	{
		CompilationResultData result;

		for (const auto& [stage, sourceInfo] : specification.shaderSourceInfo)
		{
			result.result = CompileSingle(stage, sourceInfo.source, sourceInfo.sourceEntry, specification, result);

			if (result.result != ShaderCompiler::CompilationResult::Success)
			{
				break;
			}
		}

		// #TODO_Ivar: This is a dumb hack for vertex only shaders
		if (specification.shaderSourceInfo.contains(RHI::ShaderStage::Vertex) && !specification.shaderSourceInfo.contains(RHI::ShaderStage::Pixel))
		{
			result.outputFormats.emplace_back(RHI::PixelFormat::D32_SFLOAT);
		}

		return result;
	}

	ShaderCompiler::CompilationResult VulkanShaderCompiler::CompileSingle(ShaderStage shaderStage, const std::string& source, const ShaderSourceEntry& sourceEntry, const Specification& specification, CompilationResultData& outData)
	{
		auto& data = outData.shaderData[shaderStage];

		std::string processedSource = source;

		if (!PreprocessSource(shaderStage, sourceEntry.filepath, processedSource))
		{
			return CompilationResult::PreprocessFailed;
		}

		const std::wstring wEntryPoint = ::Utility::ToWString(sourceEntry.entryPoint);
		const std::wstring renderGraphConstantsBinding = std::to_wstring(Globals::SHADER_GLOBALS_BINDING);
		const std::wstring renderGraphConstantsSpace = std::to_wstring(Globals::SHADER_GLOBALS_SPACE);

		Vector<const wchar_t*> arguments =
		{
			sourceEntry.filepath.c_str(),
			L"-E",
			wEntryPoint.c_str(),
			L"-T",
			Utility::HLSLShaderProfile(shaderStage),
			L"-spirv",
			L"-fspv-target-env=vulkan1.3",
			L"-HV",
			L"2021",
			L"-D", L"__VULKAN__ ",
			L"-enable-16bit-types",
			L"-fvk-use-dx-layout",
			L"-fvk-bind-globals", renderGraphConstantsBinding.c_str(), renderGraphConstantsSpace.c_str(),

			DXC_ARG_PACK_MATRIX_COLUMN_MAJOR
		};

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

		if (shaderStage == ShaderStage::Vertex || shaderStage == ShaderStage::Hull || shaderStage == ShaderStage::Geometry)
		{
			arguments.emplace_back(L"-fvk-invert-y");
		}

		// Pre processing
		{
			PreProcessorData processingData{};
			processingData.shaderSource = processedSource;
			processingData.shaderStage = shaderStage;
			processingData.entryPoint = sourceEntry.entryPoint;

			PreProcessorResult result{};
			if (!ShaderPreProcessor::PreProcessShaderSource(processingData, result))
			{
				return CompilationResult::PreprocessFailed;
			}

			if (shaderStage == ShaderStage::Pixel)
			{
				outData.outputFormats = result.outputFormats;
			}
			else if (shaderStage == ShaderStage::Vertex)
			{
				outData.vertexLayout = result.vertexLayout;
				outData.instanceLayout = result.instanceLayout;
			}

			processedSource = result.preProcessedResult;
		}

		IDxcBlobEncoding* sourcePtr = nullptr;
		m_hlslUtils->CreateBlob(processedSource.c_str(), static_cast<uint32_t>(processedSource.size()), CP_UTF8, &sourcePtr);

		DxcBuffer sourceBuffer{};
		sourceBuffer.Ptr = sourcePtr->GetBufferPointer();
		sourceBuffer.Size = sourcePtr->GetBufferSize();
		sourceBuffer.Encoding = 0;

		IDxcResult* compilationResult = nullptr;
		std::string error;

		HRESULT result = m_hlslCompiler->Compile(&sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()), nullptr, IID_PPV_ARGS(&compilationResult));

		const bool failed = FAILED(result);
		if (failed)
		{
			error = std::format("Failed to compile. Error: {}\n", result);
			error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compilationResult), sourceEntry.filepath.string()));
		}

		if (error.empty())
		{
			IDxcBlob* shaderResult = nullptr;
			compilationResult->GetResult(&shaderResult);

			if (!shaderResult || shaderResult->GetBufferSize() == 0)
			{
				error = std::format("Failed to compile. Error: {}\n", result);
				error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compilationResult), sourceEntry.filepath.string()));

				VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, error);

				sourcePtr->Release();
				compilationResult->Release();

				return CompilationResult::Failure;
			}

			const size_t size = shaderResult->GetBufferSize();

			data.resize_uninitialized(size / sizeof(uint32_t));
			memcpy_s(data.data(), size, shaderResult->GetBufferPointer(), shaderResult->GetBufferSize());

			shaderResult->Release();
		}
		else
		{
			sourcePtr->Release();
			compilationResult->Release();

			VT_LOGC_UNFORMATTED(Error, LogVulkanRHI, error);
			return CompilationResult::Failure;
		}

		sourcePtr->Release();
		compilationResult->Release();

		VT_LOGC(Info, LogVulkanRHI, "Successfully compiled shader {}!", sourceEntry.filepath);

		return CompilationResult::Success;
	}

	void VulkanShaderCompiler::ReflectAllStages(const Specification& specification, CompilationResultData& inOutData)
	{
		for (const auto& [stage, data] : inOutData.shaderData)
		{
			VT_LOGC(Trace, LogVulkanRHI, "Reflecting shader {0}", specification.shaderSourceInfo.at(stage).sourceEntry.filepath.string());
			ReflectStage(stage, specification, inOutData);
		}
	}

	bool IsResourceStructType(const std::string& typeName)
	{
		// Buffers
		if (typeName == "TextureSampler") return true;
		else if (typeName == "RWRawByteBuffer") return true;
		else if (typeName == "RawByteBuffer") return true;
		else if (typeName == "UniformBuffer") return true;
		else if (typeName == "RWTypedBuffer") return true;
		else if (typeName == "TypedBuffer") return true;

		// Texture2D
		else if (typeName == "RWTex2D") return true;
		else if (typeName == "Tex2D") return true;

		// Texture2DArray
		else if (typeName == "RWTex2DArray") return true;
		else if (typeName == "Tex2DArray") return true;

		// TextureCube
		else if (typeName == "TexCube") return true;

		// Texture3D
		else if (typeName == "RWTex3D") return true;
		else if (typeName == "Tex3D") return true;

		else return false;
	}

	ShaderUniformBaseType GetBaseTypeFromSPIRBaseType(spirv_cross::SPIRType::BaseType baseType)
	{
		switch (baseType)
		{
			case spirv_cross::SPIRType::Boolean: return ShaderUniformBaseType::Bool;
			case spirv_cross::SPIRType::Short: return ShaderUniformBaseType::Short;
			case spirv_cross::SPIRType::UShort: return ShaderUniformBaseType::UShort;
			case spirv_cross::SPIRType::Int: return ShaderUniformBaseType::Int;
			case spirv_cross::SPIRType::UInt: return ShaderUniformBaseType::UInt;
			case spirv_cross::SPIRType::Int64: return ShaderUniformBaseType::Int64;
			case spirv_cross::SPIRType::UInt64: return ShaderUniformBaseType::UInt64;
			case spirv_cross::SPIRType::Half: return ShaderUniformBaseType::Half;
			case spirv_cross::SPIRType::Float: return ShaderUniformBaseType::Float;
			case spirv_cross::SPIRType::Double: return ShaderUniformBaseType::Double;
		}

		VT_ENSURE_MSG(false, "Invalid type!");

		return ShaderUniformBaseType::Bool;
	}

	ShaderUniformType GetShaderUniformTypeFromSPIRType(spirv_cross::Compiler& compiler, const spirv_cross::TypeID& spirvTypeID)
	{
		ShaderUniformType resultType{};

		const spirv_cross::SPIRType& spirvType = compiler.get_type(spirvTypeID);

		if (spirvType.basetype == spirv_cross::SPIRType::BaseType::Struct)
		{
			const std::string& typeName = compiler.get_name(spirvTypeID);

			// Buffers
			if (typeName == "TextureSampler")
			{
				resultType.baseType = ShaderUniformBaseType::Sampler;
			}
			else if (typeName == "RWRawByteBuffer")
			{
				resultType.baseType = ShaderUniformBaseType::RWBuffer;
			}
			else if (typeName == "RawByteBuffer")
			{
				resultType.baseType = ShaderUniformBaseType::Buffer;
			}
			else if (typeName == "UniformBuffer")
			{
				resultType.baseType = ShaderUniformBaseType::UniformBuffer;
			}
			else if (typeName == "RWTypedBuffer")
			{
				resultType.baseType = ShaderUniformBaseType::RWBuffer;
			}
			else if (typeName == "TypedBuffer")
			{
				resultType.baseType = ShaderUniformBaseType::Buffer;
			}

			// Texture2D
			else if (typeName == "RWTex2D")
			{
				resultType.baseType = ShaderUniformBaseType::RWTexture2D;
			}
			else if (typeName == "Tex2D")
			{
				resultType.baseType = ShaderUniformBaseType::Texture2D;
			}

			// Texture2DArray
			else if (typeName == "RWTex2DArray")
			{
				resultType.baseType = ShaderUniformBaseType::RWTexture2DArray;
			}
			else if (typeName == "Tex2DArray")
			{
				resultType.baseType = ShaderUniformBaseType::Texture2DArray;
			}

			// TextureCube
			else if (typeName == "TexCube")
			{
				// #TODO_Ivar: Why is this Texture2D original implementation?
				resultType.baseType = ShaderUniformBaseType::TextureCube;
			}

			// Texture3D
			else if (typeName == "RWTex3D")
			{
				resultType.baseType = ShaderUniformBaseType::RWTexture3D;
			}
			else if (typeName == "Tex3D")
			{
				resultType.baseType = ShaderUniformBaseType::Texture3D;
			}
			else
			{
				VT_ENSURE_MSG(false, "Unknown type!");
			}
		}
		else
		{
			resultType.baseType = GetBaseTypeFromSPIRBaseType(spirvType.basetype);
			resultType.columns = spirvType.columns;
			resultType.vecsize = spirvType.vecsize;
		}

		return resultType;
	}

	void ReflectGlobalsStruct(spirv_cross::Compiler& compiler, const spirv_cross::TypeID& spirvTypeID, const std::string& parentMemberName, size_t offset, ShaderCompiler::CompilationResultData& inOutData)
	{
		const spirv_cross::SPIRType& structType = compiler.get_type(spirvTypeID);

		for (size_t m = 0; m < structType.member_types.size(); ++m)
		{
			const auto& spirvMemberTypeID = structType.member_types[m];
			const spirv_cross::SPIRType& memberType = compiler.get_type(spirvMemberTypeID);
			const std::string& memberTypeName = compiler.get_name(spirvMemberTypeID);

			const std::string memberName = compiler.get_member_name(spirvTypeID, static_cast<uint32_t>(m));
			const std::string uniformName = !parentMemberName.empty() ? parentMemberName + "." + memberName : memberName;
			const uint32_t memberOffset = compiler.type_struct_member_offset(structType, static_cast<uint32_t>(m));

			if (memberType.basetype == spirv_cross::SPIRType::BaseType::Struct && !IsResourceStructType(memberTypeName))
			{
				ReflectGlobalsStruct(compiler, spirvMemberTypeID, uniformName, offset + memberOffset, inOutData);
			}
			else
			{
				ShaderUniformType uniformType = GetShaderUniformTypeFromSPIRType(compiler, spirvMemberTypeID);
				inOutData.renderGraphConstants.uniforms[StringHash::Construct(uniformName)] = ShaderUniform(uniformType, uniformType.GetSize(), offset + memberOffset);
			}
		}
	}

	void ReflectGlobals(spirv_cross::Compiler& compiler, const spirv_cross::Resource& globalsBufferResource, ShaderCompiler::CompilationResultData& inOutData)
	{
		const auto& globalsBufferType = compiler.get_type(globalsBufferResource.base_type_id);

		inOutData.renderGraphConstants.size = compiler.get_declared_struct_size(globalsBufferType);;
		inOutData.renderGraphConstants.uniforms.clear();
		

		for (size_t i = 0; i < globalsBufferType.member_types.size(); ++i)
		{
			std::string memberName = compiler.get_member_name(globalsBufferResource.base_type_id, static_cast<uint32_t>(i));

			// If the type is a non resource struct type, we need to propagate the members out.
			const auto& spirvTypeID = globalsBufferType.member_types[i];
			const spirv_cross::SPIRType& memberType = compiler.get_type(spirvTypeID);
			const std::string& memberTypeName = compiler.get_name(spirvTypeID);
			const uint32_t memberOffset = compiler.type_struct_member_offset(globalsBufferType, static_cast<uint32_t>(i));

			if (memberType.basetype == spirv_cross::SPIRType::BaseType::Struct && !IsResourceStructType(memberTypeName))
			{
				ReflectGlobalsStruct(compiler, spirvTypeID, memberName, memberOffset, inOutData);
			}
			else
			{
				ShaderUniformType uniformType = GetShaderUniformTypeFromSPIRType(compiler, spirvTypeID);
				inOutData.renderGraphConstants.uniforms[StringHash::Construct(memberName)] = ShaderUniform(uniformType, uniformType.GetSize(), memberOffset);
			}
		}
	}

	void ReflectGlobalsStruct2(spirv_cross::Compiler& compiler, const spirv_cross::TypeID& spirvTypeID, const std::string& parentMemberName, size_t offset, ShaderCompiler::CompilationResultData2& inOutData)
	{
		const spirv_cross::SPIRType& structType = compiler.get_type(spirvTypeID);

		for (size_t m = 0; m < structType.member_types.size(); ++m)
		{
			const auto& spirvMemberTypeID = structType.member_types[m];
			const spirv_cross::SPIRType& memberType = compiler.get_type(spirvMemberTypeID);
			const std::string& memberTypeName = compiler.get_name(spirvMemberTypeID);

			const std::string memberName = compiler.get_member_name(spirvTypeID, static_cast<uint32_t>(m));
			const std::string uniformName = !parentMemberName.empty() ? parentMemberName + "." + memberName : memberName;
			const uint32_t memberOffset = compiler.type_struct_member_offset(structType, static_cast<uint32_t>(m));

			if (memberType.basetype == spirv_cross::SPIRType::BaseType::Struct && !IsResourceStructType(memberTypeName))
			{
				ReflectGlobalsStruct2(compiler, spirvMemberTypeID, uniformName, offset + memberOffset, inOutData);
			}
			else
			{
				ShaderUniformType uniformType = GetShaderUniformTypeFromSPIRType(compiler, spirvMemberTypeID);
				inOutData.shaderUniforms.uniforms[StringHash::Construct(uniformName)] = ShaderUniform(uniformType, uniformType.GetSize(), offset + memberOffset);
			}
		}
	}

	void ReflectGlobals2(spirv_cross::Compiler& compiler, const spirv_cross::Resource& globalsBufferResource, ShaderCompiler::CompilationResultData2& inOutData)
	{
		const auto& globalsBufferType = compiler.get_type(globalsBufferResource.base_type_id);

		inOutData.shaderUniforms.size = compiler.get_declared_struct_size(globalsBufferType);;
		inOutData.shaderUniforms.uniforms.clear();


		for (size_t i = 0; i < globalsBufferType.member_types.size(); ++i)
		{
			std::string memberName = compiler.get_member_name(globalsBufferResource.base_type_id, static_cast<uint32_t>(i));

			// If the type is a non resource struct type, we need to propagate the members out.
			const auto& spirvTypeID = globalsBufferType.member_types[i];
			const spirv_cross::SPIRType& memberType = compiler.get_type(spirvTypeID);
			const std::string& memberTypeName = compiler.get_name(spirvTypeID);
			const uint32_t memberOffset = compiler.type_struct_member_offset(globalsBufferType, static_cast<uint32_t>(i));

			if (memberType.basetype == spirv_cross::SPIRType::BaseType::Struct && !IsResourceStructType(memberTypeName))
			{
				ReflectGlobalsStruct2(compiler, spirvTypeID, memberName, memberOffset, inOutData);
			}
			else
			{
				ShaderUniformType uniformType = GetShaderUniformTypeFromSPIRType(compiler, spirvTypeID);
				inOutData.shaderUniforms.uniforms[StringHash::Construct(memberName)] = ShaderUniform(uniformType, uniformType.GetSize(), memberOffset);
			}
		}
	}

	void VulkanShaderCompiler::ReflectStage(ShaderStage stage, const Specification& specification, CompilationResultData& inOutData)
	{
		spirv_cross::Compiler compiler{ inOutData.shaderData[stage].data(), inOutData.shaderData[stage].size() };
		const auto resources = compiler.get_shader_resources();

		for (const auto& ubo : resources.uniform_buffers)
		{
			if (compiler.get_active_buffer_ranges(ubo.id).empty())
			{
				continue;
			}

			const auto& bufferType = compiler.get_type(ubo.base_type_id);

			const size_t size = compiler.get_declared_struct_size(bufferType);
			const uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(ubo.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			if (name == "$Globals")
			{
				ReflectGlobals(compiler, ubo, inOutData);
				continue;
			}

			auto& buffer = inOutData.uniformBuffers[set][binding];
			buffer.usageStages = buffer.usageStages | stage;
			buffer.usageCount++;
			buffer.size = size;
		}

		for (const auto& ssbo : resources.storage_buffers)
		{
			const auto& bufferBaseType = compiler.get_type(ssbo.base_type_id);
			const auto& bufferType = compiler.get_type(ssbo.type_id);

			const size_t size = compiler.get_declared_struct_size(bufferBaseType);
			const uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(ssbo.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.storageBuffers[set].contains(binding);

			auto& buffer = inOutData.storageBuffers[set][binding];
			buffer.usageStages = buffer.usageStages | stage;
			buffer.usageCount++;
			buffer.size = size;

			if (firstEntry && !bufferType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(bufferType.array[0]);

				if (arraySize == 0)
				{
					buffer.arraySize = -1;
				}
				else
				{
					buffer.arraySize = arraySize;
				}
			}
		}

		for (const auto& image : resources.storage_images)
		{
			const uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			const auto& imageType = compiler.get_type(image.type_id);
			const std::string& name = compiler.get_name(image.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.storageImages[set].contains(binding);

			auto& shaderImage = inOutData.storageImages[set][binding];
			shaderImage.usageStages = shaderImage.usageStages | stage;
			shaderImage.usageCount++;

			if (firstEntry && !imageType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(imageType.array[0]);

				if (arraySize == 0)
				{
					shaderImage.arraySize = -1;
				}
				else
				{
					shaderImage.arraySize = arraySize;
				}
			}
		}

		for (const auto& image : resources.separate_images)
		{
			const uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			const auto& imageType = compiler.get_type(image.type_id);
			const std::string& name = compiler.get_name(image.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.images[set].contains(binding);

			auto& shaderImage = inOutData.images[set][binding];
			shaderImage.usageStages = shaderImage.usageStages | stage;
			shaderImage.usageCount++;

			if (firstEntry && !imageType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(imageType.array[0]);

				if (arraySize == 0)
				{
					shaderImage.arraySize = -1;
				}
				else
				{
					shaderImage.arraySize = arraySize;
				}
			}
		}

		for (const auto& sampler : resources.separate_samplers)
		{
			const uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(sampler.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			auto& shaderSampler = inOutData.samplers[set][binding];
			shaderSampler.usageStages = shaderSampler.usageStages | stage;
			shaderSampler.usageCount++;
		}

		for (const auto& pushConstant : resources.push_constant_buffers)
		{
			const auto& bufferType = compiler.get_type(pushConstant.base_type_id);
			const size_t pushConstantSize = compiler.get_declared_struct_size(bufferType);

			inOutData.constantsBuffer.SetSize(pushConstantSize);

			inOutData.constants.size = static_cast<uint32_t>(pushConstantSize);
			inOutData.constants.offset = 0;
			inOutData.constants.stageFlags = inOutData.constants.stageFlags | stage;

			for (uint32_t i = 0; const auto & member : bufferType.member_types)
			{
				const auto& memberType = compiler.get_type(member);
				const size_t memberSize = compiler.get_declared_struct_member_size(bufferType, i);
				const size_t memberOffset = compiler.type_struct_member_offset(bufferType, i);
				const std::string& memberName = compiler.get_member_name(pushConstant.base_type_id, i);

				const auto type = Utility::GetShaderUniformTypeFromSPIRV(memberType);

				inOutData.constantsBuffer.AddMember(memberName, type, memberSize, memberOffset);
				i++;
			}
		}
	}

	bool VulkanShaderCompiler::TryAddShaderBinding(const std::string& name, uint32_t set, uint32_t binding, CompilationResultData& outData)
	{
		if (outData.bindings.contains(name))
		{
			return false;
		}

		outData.bindings[name] = { set, binding, ShaderRegisterType::UnorderedAccess };
		return true;
	}

	bool VulkanShaderCompiler::TryAddShaderBinding(const std::string& name, uint32_t set, uint32_t binding, CompilationResultData2& outData)
	{
		if (outData.bindings.contains(name))
		{
			return false;
		}

		outData.bindings[name] = { set, binding, ShaderRegisterType::UnorderedAccess };
		return true;
	}

	bool VulkanShaderCompiler::PreprocessSource(const ShaderStage shaderStage, const std::filesystem::path& filepath, std::string& outSource)
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
			filepath.c_str(),
			L"-P", // Preproccess
			L"-D", L"__HLSL__",
			L"-D", L"__VULKAN__",
		};

		if ((m_flags & ShaderCompilerFlags::WarningsAsErrors) != ShaderCompilerFlags::None)
		{
			arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		}

		for (const auto& includeDir : wcIncludeDirs)
		{
			arguments.push_back(includeDir);
		}

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

		IDxcBlobEncoding* sourcePtr = nullptr;
		m_hlslUtils->CreateBlob(outSource.c_str(), static_cast<uint32_t>(outSource.size()), CP_UTF8, &sourcePtr);

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
			error.append(std::format("{0}\nWhile compiling shader file: {1}", Utility::GetErrorStringFromResult(compilationResult), filepath.string()));
		}

		if (error.empty())
		{
			IDxcBlob* compileResult = nullptr;
			compilationResult->GetResult(&compileResult);

			outSource = reinterpret_cast<const char*>(compileResult->GetBufferPointer());
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

	void* VulkanShaderCompiler::GetHandleImpl() const
	{
		return nullptr;
	}

	ShaderCompiler::CompilationResultData2 VulkanShaderCompiler::TryCompileImpl2(const Specification2& specification)
	{
		if (!specification.forceCompile)
		{
			// #TODO_Ivar: Add shader caching later
		}

		if (specification.shaderSourceInfo.source.empty())
		{
			VT_LOGC(Error, LogVulkanRHI, "Trying to compile a shader without a source!");
			return {};
		}

		CompilationResultData2 result = CompileShader(specification);
		if (result.result != ShaderCompiler::CompilationResult::Success)
		{
			// #TODO_Ivar: Add getting from shader cache
		}

		ReflectShader(specification, result);
		// #TODO_Ivar: Cache shader

		return result;
	}

	ShaderCompiler::CompilationResultData2 VulkanShaderCompiler::CompileShader(const Specification2& specification)
	{
		CompilationResultData2 result;

		const ShaderSourceEntry& sourceEntry = specification.shaderSourceInfo.sourceEntry;
		std::string processedSource = specification.shaderSourceInfo.source;

		if (!PreprocessSource2(specification, processedSource))
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

	bool VulkanShaderCompiler::PreprocessSource2(const Specification2& specification, std::string& outProcessedSource)
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

	void VulkanShaderCompiler::ReflectShader(const Specification2& specification, CompilationResultData2& inOutData)
	{
		spirv_cross::Compiler compiler{ inOutData.shaderBinary.data(), inOutData.shaderBinary.size() };
		const auto resources = compiler.get_shader_resources();

		const ShaderStage shaderStage = specification.shaderSourceInfo.sourceEntry.shaderStage;

		for (const auto& ubo : resources.uniform_buffers)
		{
			// Skip if buffer is unused
			if (compiler.get_active_buffer_ranges(ubo.id).empty())
			{
				continue;
			}

			const auto& bufferType = compiler.get_type(ubo.base_type_id);

			const size_t size = compiler.get_declared_struct_size(bufferType);
			const uint32_t binding = compiler.get_decoration(ubo.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(ubo.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(ubo.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			if (name == "$Globals")
			{
				ReflectGlobals2(compiler, ubo, inOutData);
				continue;
			}

			auto& buffer = inOutData.uniformBuffers[set][binding];
			buffer.usageStages = shaderStage;
			buffer.usageCount++;
			buffer.size = size;
		}

		for (const auto& ssbo : resources.storage_buffers)
		{
			const auto& bufferBaseType = compiler.get_type(ssbo.base_type_id);
			const auto& bufferType = compiler.get_type(ssbo.type_id);

			const size_t size = compiler.get_declared_struct_size(bufferBaseType);
			const uint32_t binding = compiler.get_decoration(ssbo.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(ssbo.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(ssbo.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.storageBuffers[set].contains(binding);

			auto& buffer = inOutData.storageBuffers[set][binding];
			buffer.usageStages = shaderStage;
			buffer.usageCount++;
			buffer.size = size;

			if (firstEntry && !bufferType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(bufferType.array[0]);

				if (arraySize == 0)
				{
					buffer.arraySize = -1;
				}
				else
				{
					buffer.arraySize = arraySize;
				}
			}
		}

		for (const auto& image : resources.storage_images)
		{
			const uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			const auto& imageType = compiler.get_type(image.type_id);
			const std::string& name = compiler.get_name(image.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.storageImages[set].contains(binding);

			auto& shaderImage = inOutData.storageImages[set][binding];
			shaderImage.usageStages = shaderStage;
			shaderImage.usageCount++;

			if (firstEntry && !imageType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(imageType.array[0]);

				if (arraySize == 0)
				{
					shaderImage.arraySize = -1;
				}
				else
				{
					shaderImage.arraySize = arraySize;
				}
			}
		}

		for (const auto& image : resources.separate_images)
		{
			const uint32_t binding = compiler.get_decoration(image.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(image.id, spv::DecorationDescriptorSet);
			const auto& imageType = compiler.get_type(image.type_id);
			const std::string& name = compiler.get_name(image.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			const bool firstEntry = !inOutData.images[set].contains(binding);

			auto& shaderImage = inOutData.images[set][binding];
			shaderImage.usageStages = shaderStage;
			shaderImage.usageCount++;

			if (firstEntry && !imageType.array.empty())
			{
				const int32_t arraySize = static_cast<int32_t>(imageType.array[0]);

				if (arraySize == 0)
				{
					shaderImage.arraySize = -1;
				}
				else
				{
					shaderImage.arraySize = arraySize;
				}
			}
		}

		for (const auto& sampler : resources.separate_samplers)
		{
			const uint32_t binding = compiler.get_decoration(sampler.id, spv::DecorationBinding);
			const uint32_t set = compiler.get_decoration(sampler.id, spv::DecorationDescriptorSet);
			const std::string& name = compiler.get_name(sampler.id);

			TryAddShaderBinding(name, set, binding, inOutData);

			auto& shaderSampler = inOutData.samplers[set][binding];
			shaderSampler.usageStages = shaderStage;
			shaderSampler.usageCount++;
		}
	}
}
