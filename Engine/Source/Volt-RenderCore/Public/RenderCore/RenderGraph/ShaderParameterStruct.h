#pragma once

#include "RenderCore/RenderGraph/ShaderTypes.h"

#include <string>

namespace Volt
{
	struct ShaderParameterStructBase {};
}

#define BEGIN_SHADER_PARAMETER_STRUCT(structName) \
	struct structName : public Volt::ShaderParameterStructBase \
	{ \
	private: \
		typedef structName CurrentStruct; \
		inline static constexpr const char* CurrentStructName = #structName; \
		struct FirstMemberID {}; \
		typedef void* FuncPtr; \
		typedef FuncPtr (*MemberFunc)(FirstMemberID, RenderContext&, const CurrentStruct&, const std::string&); \
		static FuncPtr ProcessMember(FirstMemberID, RenderContext&, const CurrentStruct&, const std::string& parentName = "") \
		{ \
			return nullptr; \
		} \
		typedef FirstMemberID

#define END_SHADER_PARAMETER_STRUCT() \
		LastMemberID; \
		public: \
		static void zzInternal_SetMembers(RenderContext& context, const CurrentStruct& data, const std::string& parentName = "") \
		{ \
			FuncPtr(*lastFunc)(LastMemberID, RenderContext&, const CurrentStruct&, const std::string&); \
			lastFunc = ProcessMember; \
			FuncPtr ptr = (FuncPtr)lastFunc; \
			do \
			{ \
				ptr = reinterpret_cast<MemberFunc>(ptr)(FirstMemberID(), context, data, parentName); \
			} while (ptr != nullptr); \
		} \
	}; 

#define SHADER_PARAMETER_COMMON_INTERNAL(paramName) \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, RenderContext& context, const CurrentStruct& data, const std::string& parentName = "") \
	{ \
		StringHash paramStrHash = StringHash::Construct(parentName.empty() ? #paramName : parentName + "." + #paramName);\
		context.SetConstant(paramStrHash, data.paramName); \
		FuncPtr(*prevFunc)(MemberID##paramName, RenderContext&, const CurrentStruct&, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(paramName)

#define SHADER_PARAMETER_BUFFER(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphBufferHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(paramName)

#define SHADER_PARAMETER_IMAGE(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphImageHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(paramName)

#define SHADER_PARAMETER_IMAGE_MIP(type, paramName, mip) \
	MemberID##paramName; \
public: \
	RenderGraphImageHandle paramName = RenderGraphNullHandle(); \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, RenderContext& context, const CurrentStruct& data, const std::string& parentName = "") \
	{ \
		StringHash paramStrHash = StringHash::Construct(parentName.empty() ? #paramName : parentName + "." + #paramName);\
		context.SetConstant(paramStrHash, data.paramName, mip); \
		FuncPtr(*prevFunc)(MemberID##paramName, RenderContext&, const CurrentStruct&, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER_UNIFORM_BUFFER(type, paramName) \
	MemberID##paramName; \
public: \
	RenderGraphUniformBufferHandle paramName = RenderGraphNullHandle(); \
	SHADER_PARAMETER_COMMON_INTERNAL(paramName)

#define SHADER_PARAMETER_SAMPLER(type, paramName) \
	MemberID##paramName; \
public: \
	ResourceHandle paramName = Resource::Invalid; \
	SHADER_PARAMETER_COMMON_INTERNAL(paramName)

#define SHADER_PARAMETER_STRUCT(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, RenderContext& context, const CurrentStruct& data, const std::string& parentName = "") \
	{ \
		std::string completeParentName = parentName; \
		if (!completeParentName.empty()) \
		{ \
			completeParentName += "."; \
		} \
		completeParentName += #paramName; \
		type::zzInternal_SetMembers(context, data.paramName, completeParentName); \
		FuncPtr(*prevFunc)(MemberID##paramName, RenderContext&, const CurrentStruct&, const std::string&); \
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
	static FuncPtr ProcessMember(NextMemberID##paramName, RenderContext& context, const CurrentStruct& data, const std::string& parentName = "") \
	{ \
		type::zzInternal_SetMembers(context, data.paramName, parentName); \
		FuncPtr(*prevFunc)(MemberID##paramName, RenderContext&, const CurrentStruct&, const std::string&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName
