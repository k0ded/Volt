#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Shader/ShaderParameterMap.h"
#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Shader/ShaderCommon.h"
#include "RHIModule/Shader/ShaderPermutationConfig.h"
#include "RHIModule/Shader/BufferLayout.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	struct ShaderCreateInfo
	{
		std::string name;
		std::filesystem::path sourceFilepath;
		std::string entryPoint;
		ShaderStage stage;
		bool forceCompile = false;
		bool failureIsFatal = true;

		ShaderPermutationConfig permutationConfig;
	};

	struct ShaderInfo
	{
		// Pixel Shader
		Vector<PixelFormat> outputFormats;

		// Vertex Shader
		BufferLayoutMap vertexLayout;
		BufferLayout instanceLayout;
	};

	class VTRHI_API Shader : public RHIInterface
	{
	public:
		using ShaderIncludeDependencies = Vector<std::filesystem::path>;

		virtual void Reload(bool forceCompile = false) = 0;
		virtual std::string_view GetName() const = 0;
		virtual size_t GetHash() const = 0;
		virtual ShaderStage GetShaderStage() const = 0;
		virtual const ShaderParameterMap& GetParameterMap() const = 0;
		virtual const ShaderInfo& GetShaderInfo() const = 0;
		virtual const ShaderSourceInfo& GetShaderSourceInfo() const = 0;
		virtual const ShaderIncludeDependencies& GetShaderIncludeDependencies() const = 0;
		virtual bool IsValid() const = 0;

		static RefPtr<Shader> Create(const ShaderCreateInfo& createInfo);

	protected:
		Shader() = default;
		virtual ~Shader() = default;
	};
}
