#pragma once

#include "RenderCore/RenderGraph/ShaderTypes.h"
#include "RenderCore/RenderGraph2/Resources/ResourceDeclarations.h"

#include <RHIModule/Shader/ShaderCommon.h>

#include <CoreUtilities/StringHash.h>

#include <string>

namespace Volt
{
	struct ShaderParameterStructBase2 {};

	enum class ShaderParameterType2 : uint8_t
	{
		BufferSRV,
		BufferUAV,
		TextureSRV,
		TextureUAV,
		UniformBuffer,
		Sampler,
		Parameter,
		BufferAccess,
		TextureAccess,
		RenderTargets
	};

	struct ShaderParameterMetadata2
	{
		std::string name;
		StringHash hashedName;
		ShaderParameterType2 parameterType;
		RGResourceAccess resourceAccessType;
		uint32_t structOffset;
		uint32_t structSize;
	};

	struct ShaderParameterRenderTargetBindings
	{
		ShaderParameterRenderTargetBindings()
			: depthTarget(nullptr)
		{ 
			memset(renderTargets, 0, sizeof(RGTextureRef) * RHI::MAX_COLOR_ATTACHMENT_COUNT);
		}

		RGTextureRef renderTargets[RHI::MAX_COLOR_ATTACHMENT_COUNT];
		RGTextureRef depthTarget;
	};
}

#define BEGIN_SHADER_PARAMETER_STRUCT2(structName) \
	struct structName : public Volt::ShaderParameterStructBase2 \
	{ \
	private: \
		typedef structName CurrentStruct; \
		inline static constexpr const char* CurrentStructName = #structName; \
		struct FirstMemberID {}; \
		typedef void* FuncPtr; \
		typedef FuncPtr (*MemberFunc)(FirstMemberID, Vector<Volt::ShaderParameterMetadata2>&); \
		static FuncPtr ProcessMember(FirstMemberID, Vector<Volt::ShaderParameterMetadata2>&) \
		{ \
			return nullptr; \
		} \
		typedef FirstMemberID

#define END_SHADER_PARAMETER_STRUCT2() \
		LastMemberID; \
		public: \
		static void zzInternal_ProcessMembers(Vector<Volt::ShaderParameterMetadata2>& outMetadata) \
		{ \
			FuncPtr(*lastFunc)(LastMemberID, Vector<Volt::ShaderParameterMetadata2>&); \
			lastFunc = ProcessMember; \
			FuncPtr ptr = (FuncPtr)lastFunc; \
			do \
			{ \
				ptr = reinterpret_cast<MemberFunc>(ptr)(FirstMemberID(), outMetadata); \
			} while (ptr != nullptr); \
		} \
	}; 

#define SHADER_PARAMETER_COMMON_INTERNAL2(type, paramName, paramType, resourceAccess) \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata2>& outMetadata) \
	{ \
		auto& paramMetadata = outMetadata.emplace_back(); \
		paramMetadata.name = #paramName; \
		paramMetadata.hashedName = StringHash::Construct(paramMetadata.name); \
		paramMetadata.parameterType = paramType; \
		paramMetadata.structSize = sizeof(type); \
		paramMetadata.structOffset = offsetof(CurrentStruct, paramName); \
		paramMetadata.resourceAccessType = resourceAccess; \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata2>&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define RG_BUFFER_ACCESS(paramName, access) \
	MemberID##paramName; \
public: \
	Volt::RGBufferRef paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGBufferRef, paramName, Volt::ShaderParameterType2::BufferAccess, access)

#define RG_RENDER_TARGETS() \
	MemberIDrenderTargets; \
public: \
	Volt::ShaderParameterRenderTargetBindings renderTargets; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::ShaderParameterRenderTargetBindings, renderTargets, Volt::ShaderParameterType2::RenderTargets, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER2(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL2(type, paramName, Volt::ShaderParameterType2::Parameter, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_BUFFER_SRV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGBufferSRVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGBufferSRVRef, paramName, Volt::ShaderParameterType2::BufferSRV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_BUFFER_UAV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGBufferUAVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGBufferUAVRef, paramName, Volt::ShaderParameterType2::BufferUAV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_TEXTURE_SRV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGTextureSRVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGTextureSRVRef, paramName, Volt::ShaderParameterType2::TextureSRV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_TEXTURE_UAV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGTextureUAVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGTextureUAVRef, paramName, Volt::ShaderParameterType2::TextureUAV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_UNIFORM_BUFFER2(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGUniformBufferRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL2(Volt::RGUniformBufferRef, paramName, Volt::ShaderParameterType2::UniformBuffer, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_STRUCT2(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata) \
	{ \
		type::zzInternal_ProcessMembers(outMetadata, offsetof(CurrentStruct, paramName)); \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define SHADER_PARAMETER_STRUCT_INCLUDE2(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata) \
	{ \
		type::zzInternal_ProcessMembers(outMetadata, offsetof(CurrentStruct, paramName)); \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName
