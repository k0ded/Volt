#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Shader/ShaderParameterMap.h"
#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Shader/ShaderCommon.h"
#include "RHIModule/Shader/ShaderPermutationConfig.h"

namespace Volt::RHI
{
	struct ShaderBindings
	{
		std::map<uint32_t, std::map<uint32_t, ShaderConstantBuffer>> uniformBuffers;
		std::map<uint32_t, std::map<uint32_t, ShaderStorageBuffer>> storageBuffers;
		std::map<uint32_t, std::map<uint32_t, ShaderStorageImage>> storageImages;
		std::map<uint32_t, std::map<uint32_t, ShaderImage>> images;
		std::map<uint32_t, std::map<uint32_t, ShaderSampler>> samplers;
	};

	struct ShaderCreateInfo
	{
		std::string name;
		std::filesystem::path sourceFilepath;
		std::string entryPoint;
		ShaderStage stage;

		ShaderPermutationConfig permutationConfig;
	};

	class VTRHI_API Shader2 : public RHIInterface
	{
	public:
		virtual std::string_view GetName() const = 0;
		virtual size_t GetHash() const = 0;
		virtual ShaderStage GetShaderStage() const = 0;
		virtual bool IsValid() const = 0;
		virtual const ShaderParameterMap& GetParameterMap() const = 0;

		static RefPtr<Shader2> Create(const ShaderCreateInfo& createInfo);

	protected:
		Shader2() = default;
		virtual ~Shader2() = default;
	};
}
