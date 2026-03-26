#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Shader/BufferLayout.h"
#include "RHIModule/Shader/Shader.h"

namespace Volt::RHI
{
	struct PreProcessorResult
	{
		String preProcessedResult;
	
		Vector<PixelFormat> outputFormats;
		BufferLayoutMap vertexLayout;
		BufferLayout instanceLayout;
	};

	struct PreProcessorData
	{
		String shaderSource;
		String entryPoint = "main";

		ShaderStage shaderStage;
	};

	class VTRHI_API ShaderPreProcessor
	{
	public:
		static bool PreProcessShaderSource(const PreProcessorData& data, PreProcessorResult& outResult);

	private:
		static bool PreProcessPixelSource(const PreProcessorData& data, PreProcessorResult& outResult);
		static bool PreProcessVertexSource(const PreProcessorData& data, PreProcessorResult& outResult);

		static PixelFormat FindDefaultFormatFromString(StringView str);
		static PixelFormat FindFormatFromLayoutQualifier(const String& str);

		static ElementType FindDefaultElementTypeFromString(StringView str);
		static ElementType FindElementTypeFromTag(StringView tagStr);

		static ShaderUniformType FindUniformTypeFromString(StringView str);

		static void ErasePreProcessData(PreProcessorResult& outResult);

		ShaderPreProcessor() = delete;
		~ShaderPreProcessor() = delete;
	};
}
