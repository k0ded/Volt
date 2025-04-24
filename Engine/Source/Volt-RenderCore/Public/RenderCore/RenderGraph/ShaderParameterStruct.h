#pragma once

#include "RenderCore/RenderGraph/ShaderTypes.h"

#include <CoreUtilities/StringHash.h>

#include <string>

namespace Volt
{
	struct ShaderParameterStructBase {};

	enum class ShaderParameterType : uint8_t
	{
		Image,
		Buffer,
		UniformBuffer,
		Sampler,
		Parameter
	};

	struct ShaderParameterMetadata
	{
		std::string name;
		StringHash hashedName;
		ShaderParameterType parameterType;
		uint32_t structOffset;
		uint32_t structSize;
		uint32_t parentStructOffset; 
		uint32_t reflectedOffset;
	};
}

#define BEGIN_SHADER_PARAMETER_STRUCT(structName) \
	struct structName : public Volt::ShaderParameterStructBase \
	{ \
	private: \
		typedef structName CurrentStruct; \
		inline static constexpr const char* CurrentStructName = #structName; \
		struct FirstMemberID {}; \
		typedef void* FuncPtr; \
		typedef FuncPtr (*MemberFunc)(FirstMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
		static FuncPtr ProcessMember(FirstMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
		{ \
			return nullptr; \
		} \
		typedef FirstMemberID

#define END_SHADER_PARAMETER_STRUCT() \
		LastMemberID; \
		public: \
		static void zzInternal_ProcessMembers(Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
		{ \
			FuncPtr(*lastFunc)(LastMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
			lastFunc = ProcessMember; \
			FuncPtr ptr = (FuncPtr)lastFunc; \
			do \
			{ \
				ptr = reinterpret_cast<MemberFunc>(ptr)(FirstMemberID(), outMetadata, parentStructOffset, parentName); \
			} while (ptr != nullptr); \
		} \
	}; 

#define SHADER_PARAMETER_COMMON_INTERNAL(type, paramName, paramType) \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
	{ \
		auto& paramMetadata = outMetadata.emplace_back(); \
		paramMetadata.name = parentName.empty() ? #paramName : parentName + "." + #paramName; \
		paramMetadata.hashedName = StringHash::Construct(paramMetadata.name); \
		paramMetadata.parameterType = paramType; \
		paramMetadata.structSize = sizeof(type); \
		paramMetadata.structOffset = offsetof(CurrentStruct, paramName); \
		paramMetadata.parentStructOffset = parentStructOffset; \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(type, paramName, ShaderParameterType::Parameter)

#define SHADER_PARAMETER_BUFFER(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphBufferHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(RenderGraphBufferHandle, paramName, ShaderParameterType::Buffer)

#define SHADER_PARAMETER_IMAGE(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphImageHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(RenderGraphImageHandle, paramName, ShaderParameterType::Image)

#define SHADER_PARAMETER_IMAGE_MIP(type, paramName, mip) \
	MemberID##paramName; \
public: \
	RenderGraphImageHandle paramName = RenderGraphNullHandle(); \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
	{ \
		auto& paramMetadata = outMetadata.emplace_back(); \
		paramMetadata.name = parentName.empty() ? #paramName : parentName + "." + #paramName; \
		paramMetadata.hashedName = StringHash::Construct(paramMetadata.name); \
		paramMetadata.parameterType = ShaderParameterType::Image; \
		paramMetadata.structSize = sizeof(RenderGraphImageHandle); \
		paramMetadata.structOffset = offsetof(CurrentStruct, paramName); \
		paramMetadata.parentStructOffset = parentStructOffset; \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER_UNIFORM_BUFFER(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphUniformBufferHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(RenderGraphUniformBufferHandle, paramName, ShaderParameterType::UniformBuffer)

#define SHADER_PARAMETER_SAMPLER(type, paramName) \
	MemberID##paramName; \
public: \
	ResourceHandle paramName = Resource::Invalid; \
	SHADER_PARAMETER_COMMON_INTERNAL(ResourceHandle, paramName, ShaderParameterType::Sampler)

#define SHADER_PARAMETER_STRUCT(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
	{ \
		std::string completeParentName = parentName; \
		if (!completeParentName.empty()) \
		{ \
			completeParentName += "."; \
		} \
		completeParentName += #paramName; \
		type::zzInternal_ProcessMembers(outMetadata, parentStructOffset + offsetof(CurrentStruct, paramName), completeParentName); \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER_STRUCT_INCLUDE(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t parentStructOffset = 0, const std::string& parentName = "") \
	{ \
		type::zzInternal_ProcessMembers(outMetadata, parentStructOffset + offsetof(CurrentStruct, paramName), parentName); \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName
