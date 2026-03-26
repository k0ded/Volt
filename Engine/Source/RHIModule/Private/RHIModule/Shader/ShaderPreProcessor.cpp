#include "rhipch.h"
#include "RHIModule/Shader/ShaderPreProcessor.h"

#include <CoreUtilities/String/StringUtility.h>

#define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

namespace Volt::RHI
{
	namespace Utility
	{
		inline static bool IsDefaultType(StringView str)
		{
			static Vector<StringView> baseTypes =
			{
				"bool",
				"int",
				"uint",
				"dword",
				"half",
				"float",
				"double",
				"min16float",
				"min10float",
				"min16int",
				"min12int",
				"min16uint",
				"uint64_t",
				"int64_t",
				"float16_t",
				"uint16_t",
				"int16_t"
			};

			for (const auto& baseType : baseTypes)
			{
				if (str == baseType)
				{
					return true;
				}

				for (uint32_t i = 2; i <= 4; i++)
				{
					if (str == FormatString("{}{}", baseType, i))
					{
						return true;
					}

					for (uint32_t j = 1; j <= 4; j++)
					{
						if (str == FormatString("{}{}x{}", baseType, i, j))
						{
							return true;
						}
					}
				}
			}

			return false;
		}

		inline static String ToLower(const String& str)
		{
			String newStr(str);
			std::transform(str.begin(), str.end(), newStr.begin(), [](unsigned char c) { return (uint8_t)std::tolower((int32_t)c); });

			return newStr;
		}

		inline static void RemoveAllNonLettNumCharacters(String& outResult)
		{
			auto newEnd = std::remove_if(outResult.begin(), outResult.end(), [](char c)
			{
				String cStr{ c };
				if (cStr.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_") != String::npos)
				{
					return true;
				}

				return false;
			});

			outResult.erase(newEnd, outResult.end());
		}

		inline static const bool IsSystemValueSemantic(StringView semanticName)
		{
			static constexpr StringView svSemantics[] =
			{
				"sv_clipdistance",
				"sv_culldistance",
				"sv_coverage",
				"sv_depth",
				"sv_depthgreaterequal",
				"sv_depthlessequal",
				"sv_dispatchthreadid",
				"sv_domainlocation",
				"sv_groupindex",
				"sv_groupthreadid",
				"sv_gsInstanceid",
				"sv_innercoverage",
				"sv_insidetessfactor",
				"sv_instanceid",
				"sv_isfrontface",
				"sv_position",
				"sv_primitiveid",
				"sv_rendertargetarrayindex",
				"sv_sampleindex",
				"sv_target",
				"sv_tessfactor",
				"sv_vertexid",
				"sv_viewportarrayindex",
				"sv_shadingrate",
			};

			const String strLower = ToLower(String(semanticName));

			for (auto svSemantic : svSemantics)
			{
				if (strLower == svSemantic)
				{
					return true;
				}
			}

			return false;
		}

		inline static const bool IsVulkanBuiltIn(StringView valueStr)
		{
			return valueStr.find("[[vk::builtin") != StringView::npos;
		}

		inline static const bool IsResourceType(StringView valueStr)
		{
			if (valueStr.find("TextureSampler") != StringView::npos ||
				valueStr.find("RawByteBuffer") != StringView::npos ||
				valueStr.find("RWRawByteBuffer") != StringView::npos ||
				valueStr.find("UniformBuffer") != StringView::npos ||
				valueStr.find("TypedBuffer") != StringView::npos ||
				valueStr.find("RWTypedBuffer") != StringView::npos ||
				valueStr.find("TTexture") != StringView::npos ||
				valueStr.find("RWTexture") != StringView::npos ||
				valueStr.find("UniformRawByteBuffer") != StringView::npos ||
				valueStr.find("UniformRWRawByteBuffer") != StringView::npos ||
				valueStr.find("UniformTypedBuffer") != StringView::npos ||
				valueStr.find("UniformRWTypedBuffer") != StringView::npos ||

				valueStr.find("UniformTex2D") != StringView::npos ||
				valueStr.find("UniformRWTex2D") != StringView::npos ||
				valueStr.find("Tex2D") != StringView::npos ||
				valueStr.find("RWTex2D") != StringView::npos ||
				
				valueStr.find("UniformTex2DArray") != StringView::npos ||
				valueStr.find("UniformRWTex2DArray") != StringView::npos ||
				valueStr.find("Tex2DArray") != StringView::npos ||
				valueStr.find("RWTex2DArray") != StringView::npos ||

				valueStr.find("UniformTexCube") != StringView::npos ||
				valueStr.find("TexCube") != StringView::npos ||
				
				valueStr.find("UniformTex3D") != StringView::npos ||
				valueStr.find("UniformRWTex3D") != StringView::npos ||
				valueStr.find("Tex3D") != StringView::npos ||
				valueStr.find("RWTex3D") != StringView::npos)
			{
				return true;
			}
		
			return false;
		}
	}

	bool ShaderPreProcessor::PreProcessShaderSource(const PreProcessorData& data, PreProcessorResult& outResult)
	{
		switch (data.shaderStage)
		{
			case ShaderStage::Pixel: return PreProcessPixelSource(data, outResult); break;
			case ShaderStage::Vertex: return PreProcessVertexSource(data, outResult); break;

			case ShaderStage::Hull:
			case ShaderStage::Domain:
			case ShaderStage::Geometry:
			case ShaderStage::Compute:
			case ShaderStage::RayGen:
			case ShaderStage::AnyHit:
			case ShaderStage::ClosestHit:
			case ShaderStage::Miss:
			case ShaderStage::Intersection:
			case ShaderStage::Amplification:
			case ShaderStage::Mesh:
			case ShaderStage::All:
			case ShaderStage::Common:
				break;
			default:
				break;
		}

		outResult.preProcessedResult = data.shaderSource;
		ErasePreProcessData(outResult);
		return true;
	}

	bool ShaderPreProcessor::PreProcessPixelSource(const PreProcessorData& data, PreProcessorResult& outResult)
	{
		const auto& entryPoint = data.entryPoint;
		outResult.preProcessedResult = data.shaderSource;
		ErasePreProcessData(outResult);

		String processedSource = data.shaderSource;

		const size_t entryPointLocation = processedSource.find(entryPoint);
		if (entryPointLocation == String::npos)
		{
			VT_LOGC(Error, LogRHI, "Unable to find Entry Point {0} in shader!", entryPoint);
			return false;
		}

		// Find return value of "main" function
		String entryPointSubStr = processedSource.substr(0, entryPointLocation);

		const size_t lastSpace = entryPointSubStr.find_last_of(' ');
		String outputSubStr = entryPointSubStr.substr(0, lastSpace);

		const size_t preReturnValueChar = outputSubStr.find_last_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
		String returnValueStr = outputSubStr.substr(preReturnValueChar + 1);

		// Check return value type and find it if necessary
		const bool isDefaultType = Utility::IsDefaultType(returnValueStr);

		if (isDefaultType)
		{
			String fullTypeSubStr = outputSubStr.substr(0, outputSubStr.size() - returnValueStr.size());

			// Remove all prefix characters
			while (fullTypeSubStr[fullTypeSubStr.size() - 1] != ']' && !fullTypeSubStr.empty())
			{
				if (fullTypeSubStr[fullTypeSubStr.size() - 1] != '\n' && fullTypeSubStr[fullTypeSubStr.size() - 1] != ' ')
				{
					break;
				}

				fullTypeSubStr.pop_back();
			}

			if (fullTypeSubStr.empty())
			{
				return false; // #TODO_Ivar: Handle
			}

			const size_t lastChar = fullTypeSubStr.find_last_not_of("[[]]abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890:");
			const String lastSubStr = fullTypeSubStr.substr(lastChar + 1);

			if (lastSubStr.empty())
			{
				outResult.outputFormats.emplace_back(FindDefaultFormatFromString(returnValueStr));
				return true;
			}
			else
			{
				outResult.outputFormats.emplace_back(FindFormatFromLayoutQualifier(lastSubStr));
			}

			return true;
		}

		const size_t structPos = outputSubStr.find("struct " + returnValueStr);
		if (structPos == String::npos)
		{
			return false;
		}

		String structSubStr = outputSubStr.substr(structPos);
		size_t openBracketPos = structSubStr.find_first_of('{') + 1;
		const size_t closingBracketPos = structSubStr.find_first_of('}');

		openBracketPos = structSubStr.find_first_not_of("\n ", openBracketPos);

		String structBracketSubStr = structSubStr.substr(openBracketPos, closingBracketPos - openBracketPos);

		size_t currentSemicolon = structBracketSubStr.find_first_of(';');

		while (currentSemicolon != String::npos)
		{
			String outputTypeStr = structBracketSubStr.substr(0, currentSemicolon);
			while (outputTypeStr[0] == ' ' || outputTypeStr[0] == '\n')
			{
				outputTypeStr.erase(outputTypeStr.begin());
			}

			const size_t firstSpace = outputTypeStr.find_first_of(' ');
			String typeStr = outputTypeStr.substr(0, firstSpace);

			const size_t qualifierPos = typeStr.find_first_of("[");

			if (qualifierPos == String::npos)
			{
				outResult.outputFormats.emplace_back(FindDefaultFormatFromString(typeStr));
			}
			else
			{
				String qualifierStr = typeStr.substr(qualifierPos, qualifierPos + typeStr.find_last_of("]") + 1);
				outResult.outputFormats.emplace_back(FindFormatFromLayoutQualifier(qualifierStr));
			}

			structBracketSubStr = structBracketSubStr.substr(currentSemicolon + 1);
			currentSemicolon = structBracketSubStr.find_first_of(';');
		}

		return true;
	}

	bool ShaderPreProcessor::PreProcessVertexSource(const PreProcessorData& data, PreProcessorResult& outResult)
	{
		const auto& entryPoint = data.entryPoint;
		outResult.preProcessedResult = data.shaderSource;
		ErasePreProcessData(outResult);

		String processedSource = data.shaderSource;

		const size_t entryPointLocation = processedSource.find(entryPoint);
		if (entryPointLocation == String::npos)
		{
			VT_LOGC(Error, LogRHI, "Unable to find Entry Point {0} in shader!", entryPoint);
			return false;
		}

		// Find entry point parentheses
		const size_t openParenthesesLoc = processedSource.find_first_of('(', entryPointLocation) + 1;
		const size_t closeParenthesesLoc = processedSource.find_first_of(')', entryPointLocation);

		const String parenthesesSubStr = processedSource.substr(openParenthesesLoc, closeParenthesesLoc - openParenthesesLoc);

		const auto arguments = ::Utility::SplitStringsByCharacter(parenthesesSubStr, ' ');
		String inputStruct;

		// #TODO: Handle non struct inputs
		for (const auto& arg : arguments)
		{
			const size_t argLoc = processedSource.find("struct " + arg);
			if (argLoc != String::npos)
			{
				inputStruct = arg;
				break;
			}
		}

		// There are no vertex inputs
		if (inputStruct.empty())
		{
			return true;
		}

		// Find the correct declaration
		size_t inputStructLoc = processedSource.find("struct " + inputStruct + " ");
		if (inputStructLoc == String::npos)
		{
			inputStructLoc = processedSource.find("struct " + inputStruct + "\n");
		}
		if (inputStructLoc == String::npos)
		{
			inputStructLoc = processedSource.find("struct " + inputStruct + "\0");
		}

		const size_t openBracketLoc = processedSource.find_first_of('{', inputStructLoc);
		const size_t closeBracketLoc = processedSource.find("};", inputStructLoc);

		String structSubStr = processedSource.substr(openBracketLoc, closeBracketLoc - openBracketLoc);

		Map<uint32_t, Vector<BufferElement>> inputElementsMap{};
		Vector<BufferElement> instanceInputElements{};

		size_t currentInputSemiColLoc = structSubStr.find_first_of(';');
		while (currentInputSemiColLoc != String::npos)
		{
			String currentValueStr = structSubStr.substr(0, currentInputSemiColLoc);

			const size_t divLoc = currentValueStr.find_last_of(':');
			if (divLoc == String::npos)
			{
				break;
			}

			String nameStr = currentValueStr.substr(divLoc);
			Utility::RemoveAllNonLettNumCharacters(nameStr);

			currentValueStr = currentValueStr.substr(0, divLoc);

			if (!Utility::IsSystemValueSemantic(nameStr) && !Utility::IsVulkanBuiltIn(currentValueStr))
			{
				ElementType elementType = ElementType::Invalid;
				uint32_t vertexInputIndex = 0;
				bool isPerInstance = false;
				bool isInvalid = false;

				size_t typeTagLoc = currentValueStr.find("[[vt::");
				while (typeTagLoc != String::npos)
				{
					String tagSubstr = currentValueStr.substr(typeTagLoc, currentValueStr.find_first_of("]]", typeTagLoc) + 2 - typeTagLoc);
					const String lowerStr = Utility::ToLower(tagSubstr);

					if (lowerStr == "[[vt::instance]]")
					{
						isPerInstance = true;
					}
					else if (lowerStr.find("vt::inputindex") != String::npos)
					{
						size_t delimiterBegin = lowerStr.find_first_of('(');
						size_t delimiterEnd = lowerStr.find_last_of(')');

						if (delimiterBegin != String::npos && delimiterEnd != String::npos)
						{
							vertexInputIndex = StoI(lowerStr.substr(delimiterBegin + 1, delimiterBegin - delimiterEnd));
						}
					}
					else
					{
						elementType = FindElementTypeFromTag(lowerStr);

						// If all checks have failed, and no element type was found, the tag is invalid.
						if (elementType == ElementType::Invalid)
						{
							VT_LOGC(Error, LogRHI, "The tag {} is not a valid vertex definition tag!", tagSubstr);
							isInvalid = true;
						}
					}

					constexpr uint32_t TAG_LENGTH = 5;
					typeTagLoc = currentValueStr.find("[[vt::", typeTagLoc + TAG_LENGTH);
				}

				if (elementType == ElementType::Invalid)
				{
					elementType = FindDefaultElementTypeFromString(currentValueStr);
				}

				// If the input declaration was invalid, we do not add it to the vertex input definition.
				if (!isInvalid)
				{
					if (!isPerInstance)
					{
						inputElementsMap[vertexInputIndex].emplace_back(elementType, nameStr);
					}
					else
					{
						instanceInputElements.emplace_back(elementType, nameStr);
					}
				}
			}

			structSubStr = structSubStr.substr(currentInputSemiColLoc + 1);
			currentInputSemiColLoc = structSubStr.find_first_of(';');
		}

		for (const auto& [index, inputElements] : inputElementsMap)
		{
			outResult.vertexLayout[index] = inputElements;
		}

		outResult.instanceLayout = instanceInputElements;

		return true;
	}

	void ShaderPreProcessor::ErasePreProcessData(PreProcessorResult& outResult)
	{
		String& result = outResult.preProcessedResult;

		size_t currentTagPos = result.find("[[vt::");

		while (currentTagPos != String::npos)
		{
			size_t endBracketPos = result.find("]]", currentTagPos);

			result.erase(currentTagPos, endBracketPos - currentTagPos + 2);

			currentTagPos = result.find("[[vt::");
		}
	}

	ElementType ShaderPreProcessor::FindDefaultElementTypeFromString(StringView str)
	{
		if (str.find(" half ") != StringView::npos)
		{
			return ElementType::Half;
		}
		else if (str.find(" half2 ") != StringView::npos)
		{
			return ElementType::Half2;
		}
		else if (str.find(" half3 ") != StringView::npos)
		{
			return ElementType::Half3;
		}
		else if (str.find(" half4 ") != StringView::npos)
		{
			return ElementType::Half4;
		}
		else if (str.find(" float ") != StringView::npos)
		{
			return ElementType::Float;
		}
		else if (str.find(" float2 ") != StringView::npos)
		{
			return ElementType::Float2;
		}
		else if (str.find(" float3 ") != StringView::npos)
		{
			return ElementType::Float3;
		}
		else if (str.find(" float4 ") != StringView::npos)
		{
			return ElementType::Float4;
		}
		else if (str.find(" int ") != StringView::npos)
		{
			return ElementType::Int;
		}
		else if (str.find(" int2 ") != StringView::npos)
		{
			return ElementType::Int2;
		}
		else if (str.find(" int3 ") != StringView::npos)
		{
			return ElementType::Int3;
		}
		else if (str.find(" int4 ") != StringView::npos)
		{
			return ElementType::Int4;
		}
		else if (str.find(" uint ") != StringView::npos)
		{
			return ElementType::UInt;
		}
		else if (str.find(" uint2 ") != StringView::npos)
		{
			return ElementType::UInt2;
		}
		else if (str.find(" uint3 ") != StringView::npos)
		{
			return ElementType::UInt3;
		}
		else if (str.find(" uint4 ") != StringView::npos)
		{
			return ElementType::UInt4;
		}
		else if (str.find(" float3x3 ") != StringView::npos)
		{
			return ElementType::Float3x3;
		}
		else if (str.find(" float4x4 ") != StringView::npos)
		{
			return ElementType::Float4x4;
		}

		return ElementType::Bool;
	}

	ElementType ShaderPreProcessor::FindElementTypeFromTag(StringView str)
	{
		if (str.find("[[vt::half]]") != StringView::npos)
		{
			return ElementType::Half;
		}
		else if (str.find("[[vt::half2]]") != StringView::npos)
		{
			return ElementType::Half2;
		}
		else if (str.find("[[vt::half3]]") != StringView::npos)
		{
			return ElementType::Half3;
		}
		else if (str.find("[[vt::half4]]") != StringView::npos)
		{
			return ElementType::Half4;
		}

		else if (str.find("[[vt::byte]]") != StringView::npos)
		{
			return ElementType::Byte;
		}
		else if (str.find("[[vt::byte2]]") != StringView::npos)
		{
			return ElementType::Byte2;
		}
		else if (str.find("[[vt::byte3]]") != StringView::npos)
		{
			return ElementType::Byte3;
		}
		else if (str.find("[[vt::byte4]]") != StringView::npos)
		{
			return ElementType::Byte4;
		}

		else if (str.find("[[vt::ushort]]") != StringView::npos)
		{
			return ElementType::UShort;
		}
		else if (str.find("[[vt::ushort2]]") != StringView::npos)
		{
			return ElementType::UShort2;
		}
		else if (str.find("[[vt::ushort3]]") != StringView::npos)
		{
			return ElementType::UShort3;
		}
		else if (str.find("[[vt::ushort4]]") != StringView::npos)
		{
			return ElementType::UShort4;
		}

		else if (str.find("[[vt::float]]") != StringView::npos)
		{
			return ElementType::Float;
		}
		else if (str.find("[[vt::float2]]") != StringView::npos)
		{
			return ElementType::Float2;
		}
		else if (str.find("[[vt::float3]]") != StringView::npos)
		{
			return ElementType::Float3;
		}
		else if (str.find("[[vt::float4]]") != StringView::npos)
		{
			return ElementType::Float4;
		}

		else if (str.find("[[vt::int]]") != StringView::npos)
		{
			return ElementType::Int;
		}
		else if (str.find("[[vt::int2]]") != StringView::npos)
		{
			return ElementType::Int2;
		}
		else if (str.find("[[vt::int3]]") != StringView::npos)
		{
			return ElementType::Int3;
		}
		else if (str.find("[[vt::int4]]") != StringView::npos)
		{
			return ElementType::Int4;
		}

		else if (str.find("[[vt::uint]]") != StringView::npos)
		{
			return ElementType::UInt;
		}
		else if (str.find("[[vt::uint2]]") != StringView::npos)
		{
			return ElementType::UInt2;
		}
		else if (str.find("[[vt::uint3]]") != StringView::npos)
		{
			return ElementType::UInt3;
		}
		else if (str.find("[[vt::uint4]]") != StringView::npos)
		{
			return ElementType::UInt4;
		}

		else if (str.find("[[vt::float3x3]]") != StringView::npos)
		{
			return ElementType::Float3x3;
		}
		else if (str.find("[[vt::float4x4]]") != StringView::npos)
		{
			return ElementType::Float4x4;
		}

		return ElementType::Invalid;
	}

	ShaderUniformType ShaderPreProcessor::FindUniformTypeFromString(StringView str)
	{
		ShaderUniformType resultType{};
		bool isResourceType = false;

		if (str.find("TextureSampler") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Sampler;
			isResourceType = true;
		}

		// Note: The order of the ifs matter, because we are using find, Tex2D will be found in a string containing RWTex2D
		// which will lead to mislabling of the type.

		// Buffers
		else if (str.find("RWRawByteBuffer") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::RWBuffer;
			isResourceType = true;
		}
		else if (str.find("RawByteBuffer") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Buffer;
			isResourceType = true;
		}
		else if (str.find("UniformBuffer") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::UniformBuffer;
			isResourceType = true;
		}
		else if (str.find("RWTypedBuffer") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::RWBuffer;
			isResourceType = true;
		}
		else if (str.find("TypedBuffer") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Buffer;
			isResourceType = true;
		}

		// Texture2D
		else if (str.find("RWTex2D") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::RWTexture2D;
			isResourceType = true;
		}
		else if (str.find("Tex2D") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Texture2D;
			isResourceType = true;
		}

		// Texture2DArray
		else if (str.find("RWTex2DArray") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::RWTexture2DArray;
			isResourceType = true;
		}
		else if (str.find("Tex2DArray") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Texture2DArray;
			isResourceType = true;
		}

		// TextureCube
		else if (str.find("TexCube") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Texture2D;
			isResourceType = true;
		}

		// Texture3D
		else if (str.find("RWTex3D") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::RWTexture3D;
			isResourceType = true;
		}
		else if (str.find("Tex3D") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Texture3D;
			isResourceType = true;
		}

		else if (str.find("bool") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Bool;
		}
		else if (str.find("int16_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Short;
		}
		else if (str.find("uint16_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::UShort;
		}
		else if (str.find("int64_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Int64;
		}
		else if (str.find("uint64_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::UInt64;
		}
		else if (str.find("uint32_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::UInt;
		}
		else if (str.find("int32_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Int;
		}
		else if (str.find("float64_t") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Double;
		}
		else if (str.find("uint") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::UInt;
		}
		else if (str.find("int") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Int;
		}
		else if (str.find("double") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Double;
		}
		else if (str.find("float") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Float;
		}
		else if (str.find("half") != StringView::npos)
		{
			resultType.baseType = ShaderUniformBaseType::Half;
		}

		if (!isResourceType)
		{
			size_t tTypeOffset = str.find("_t");
			size_t findOffset = 0;

			if (tTypeOffset != StringView::npos)
			{
				findOffset = tTypeOffset;
			}

			size_t lastNumOffset = str.find_first_not_of("abcdefghijklmnopqrstuvwxyz<>[]", findOffset);
			if (lastNumOffset != StringView::npos)
			{
				StringView postfixStr = str.substr(lastNumOffset, str.size() - lastNumOffset);

				const uint32_t vecSize = static_cast<uint32_t>(StoI(String(1, postfixStr[0])));
				resultType.vecsize = vecSize;

				if (postfixStr.size() > 1)
				{
					const uint32_t columnCount = static_cast<uint32_t>(StoI(String(1, postfixStr[postfixStr.size() - 1])));
					resultType.columns = columnCount;
				}
			}
		}

		return resultType;
	}

	PixelFormat ShaderPreProcessor::FindDefaultFormatFromString(StringView str)
	{
		if (str == "float")
		{
			return PixelFormat::R32_SFLOAT;
		}
		else if (str == "float2")
		{
			return PixelFormat::R32G32_SFLOAT;
		}
		else if (str == "float3")
		{
			return PixelFormat::R32G32B32_SFLOAT;
		}
		else if (str == "float4")
		{
			return PixelFormat::R32G32B32A32_SFLOAT;
		}
		else if (str == "int")
		{
			return PixelFormat::R32_SINT;
		}
		else if (str == "int2")
		{
			return PixelFormat::R32G32_SINT;
		}
		else if (str == "int3")
		{
			return PixelFormat::R32G32B32_SINT;
		}
		else if (str == "int4")
		{
			return PixelFormat::R32G32B32A32_SINT;
		}
		else if (str == "uint")
		{
			return PixelFormat::R32_UINT;
		}
		else if (str == "uint2")
		{
			return PixelFormat::R32G32_UINT;
		}
		else if (str == "uint3")
		{
			return PixelFormat::R32G32B32_UINT;
		}
		else if (str == "uint4")
		{
			return PixelFormat::R32G32B32A32_UINT;
		}
		else if (str == "half")
		{
			return PixelFormat::R16_SFLOAT;
		}
		else if (str == "half2")
		{
			return PixelFormat::R16G16_SFLOAT;
		}
		else if (str == "half3")
		{
			return PixelFormat::R16G16B16_SFLOAT;
		}
		else if (str == "half4")
		{
			return PixelFormat::R16G16B16A16_SFLOAT;
		}

		VT_LOGC(Error, LogRHI, "Unable to translate type {0} into any format!", str);
		return PixelFormat::UNDEFINED;
	}

	PixelFormat ShaderPreProcessor::FindFormatFromLayoutQualifier(const String& layoutStr)
	{
		String tempStr = layoutStr;
		tempStr.erase(std::remove(tempStr.begin(), tempStr.end(), '['), tempStr.end());
		tempStr.erase(std::remove(tempStr.begin(), tempStr.end(), ']'), tempStr.end());

		tempStr = tempStr.substr(4); // Remove "vt::"
		tempStr = Utility::ToLower(tempStr);

		// Floating point
		if (tempStr == "rgba32f")
		{
			return PixelFormat::R32G32B32A32_SFLOAT;
		}
		else if (tempStr == "rgb32f")
		{
			return PixelFormat::R32G32B32_SFLOAT;
		}
		else if (tempStr == "rg32f")
		{
			return PixelFormat::R32G32_SFLOAT;
		}
		else if (tempStr == "r32f")
		{
			return PixelFormat::R32_SFLOAT;
		}
		else if (tempStr == "rgba16f")
		{
			return PixelFormat::R16G16B16A16_SFLOAT;
		}
		else if (tempStr == "rgb16f")
		{
			return PixelFormat::R16G16B16_SFLOAT;
		}
		else if (tempStr == "rg16f")
		{
			return PixelFormat::R16G16_SFLOAT;
		}
		else if (tempStr == "r16f")
		{
			return PixelFormat::R16_SFLOAT;
		}
		else if (tempStr == "rgba16")
		{
			return PixelFormat::R16G16B16A16_UNORM;
		}
		else if (tempStr == "rgb16")
		{
			return PixelFormat::R16G16B16_UNORM;
		}
		else if (tempStr == "rg16")
		{
			return PixelFormat::R16G16_UNORM;
		}
		else if (tempStr == "r16")
		{
			return PixelFormat::R16_UNORM;
		}
		else if (tempStr == "rgba8")
		{
			return PixelFormat::R8G8B8A8_UNORM;
		}
		else if (tempStr == "rgb8")
		{
			return PixelFormat::R8G8B8_UNORM;
		}
		else if (tempStr == "rg8")
		{
			return PixelFormat::R8G8_UNORM;
		}
		else if (tempStr == "r8")
		{
			return PixelFormat::R8_UNORM;
		}
		else if (tempStr == "rgba16_snorm")
		{
			return PixelFormat::R16G16B16A16_SNORM;
		}
		else if (tempStr == "rgb16_snorm")
		{
			return PixelFormat::R16G16B16_SNORM;
		}
		else if (tempStr == "rg16_snorm")
		{
			return PixelFormat::R16G16_SNORM;
		}
		else if (tempStr == "r16_snorm")
		{
			return PixelFormat::R16_SNORM;
		}
		else if (tempStr == "rgba8_snorm")
		{
			return PixelFormat::R8G8B8A8_SNORM;
		}
		else if (tempStr == "rgb8_snorm")
		{
			return PixelFormat::R8G8B8_SNORM;
		}
		else if (tempStr == "rg8_snorm")
		{
			return PixelFormat::R8G8_SNORM;
		}
		else if (tempStr == "r8_snorm")
		{
			return PixelFormat::R8_SNORM;
		}
		else if (tempStr == "r11f_g11f_b10f")
		{
			return PixelFormat::B10G11R11_UFLOAT_PACK32;
		}
		else if (tempStr == "rgb10_a2")
		{
			return PixelFormat::A2B10G10R10_UNORM_PACK32;
		}

		// Signed int
		if (tempStr == "rgba32i")
		{
			return PixelFormat::R32G32B32A32_SINT;
		}
		else if (tempStr == "rgb32i")
		{
			return PixelFormat::R32G32B32_SINT;
		}
		else if (tempStr == "rg32i")
		{
			return PixelFormat::R32G32_SINT;
		}
		else if (tempStr == "r32i")
		{
			return PixelFormat::R32_SINT;
		}
		else if (tempStr == "rgba16i")
		{
			return PixelFormat::R16G16B16A16_SINT;
		}
		else if (tempStr == "rgb16i")
		{
			return PixelFormat::R16G16B16_SINT;
		}
		else if (tempStr == "rg16i")
		{
			return PixelFormat::R16G16_SINT;
		}
		else if (tempStr == "r16i")
		{
			return PixelFormat::R16_SINT;
		}
		else if (tempStr == "rgba8i")
		{
			return PixelFormat::R8G8B8A8_SINT;
		}
		else if (tempStr == "rgb8i")
		{
			return PixelFormat::R8G8B8_SINT;
		}
		else if (tempStr == "rg8i")
		{
			return PixelFormat::R8G8_SINT;
		}
		else if (tempStr == "r8i")
		{
			return PixelFormat::R8_SINT;
		}

		// Unsigned int
		if (tempStr == "rgba32ui")
		{
			return PixelFormat::R32G32B32A32_UINT;
		}
		else if (tempStr == "rgb32ui")
		{
			return PixelFormat::R32G32B32_UINT;
		}
		else if (tempStr == "rg32ui")
		{
			return PixelFormat::R32G32_UINT;
		}
		else if (tempStr == "r32ui")
		{
			return PixelFormat::R32_UINT;
		}
		else if (tempStr == "rgba16ui")
		{
			return PixelFormat::R16G16B16A16_UINT;
		}
		else if (tempStr == "rgb16ui")
		{
			return PixelFormat::R16G16B16_UINT;
		}
		else if (tempStr == "rg16ui")
		{
			return PixelFormat::R16G16_UINT;
		}
		else if (tempStr == "r16ui")
		{
			return PixelFormat::R16_UINT;
		}
		else if (tempStr == "rgba8ui")
		{
			return PixelFormat::R8G8B8A8_UINT;
		}
		else if (tempStr == "rgb8ui")
		{
			return PixelFormat::R8G8B8_UINT;
		}
		else if (tempStr == "rg8ui")
		{
			return PixelFormat::R8G8_UINT;
		}
		else if (tempStr == "r8ui")
		{
			return PixelFormat::R8_UINT;
		}
		else if (tempStr == "rgb10_a2ui")
		{
			return PixelFormat::A2B10G10R10_UINT_PACK32;
		}

		// Depth
		if (tempStr == "d32f")
		{
			return PixelFormat::D32_SFLOAT;
		}
		else if (tempStr == "d16u")
		{
			return PixelFormat::D16_UNORM;
		}
		else if (tempStr == "d16us8")
		{
			return PixelFormat::D16_UNORM_S8_UINT;
		}
		else if (tempStr == "d24us8")
		{
			return PixelFormat::D24_UNORM_S8_UINT;
		}
		else if (tempStr == "d32fs8")
		{
			return PixelFormat::D32_SFLOAT_S8_UINT;
		}

		VT_LOGC(Error, LogRHI, "Unable to translate layout qualifier {0} into any format!", tempStr);
		return PixelFormat::UNDEFINED;
	}
}
