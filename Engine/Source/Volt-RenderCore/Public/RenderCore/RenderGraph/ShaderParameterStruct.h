#pragma once

#include "RenderCore/RenderGraph/ShaderTypes.h"
#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/Shader/GlobalShader.h"

#include <RHIModule/Shader/ShaderCommon.h>
#include <RHIModule/Images/SamplerState.h>

#include <CoreUtilities/StringHash.h>

#include <string>

namespace Volt
{
	struct ShaderParameterStructBase {};

	enum class ShaderParameterType : uint8_t
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
		UniformBufferAccess,
		RenderTargets
	};

	struct ShaderParameterMetadata
	{
		std::string name;
		StringHash hashedName;
		ShaderParameterType parameterType;
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

#define BEGIN_SHADER_PARAMETER_STRUCT(structName) \
	struct structName : public Volt::ShaderParameterStructBase \
	{ \
	private: \
		typedef structName CurrentStruct; \
		inline static constexpr const char* CurrentStructName = #structName; \
		struct FirstMemberID {}; \
		typedef void* FuncPtr; \
		typedef FuncPtr (*MemberFunc)(FirstMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t); \
		static FuncPtr ProcessMember(FirstMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t) \
		{ \
			return nullptr; \
		} \
		typedef FirstMemberID

#define END_SHADER_PARAMETER_STRUCT() \
		LastMemberID; \
		public: \
		static void zzInternal_ProcessMembers(Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t offset = 0) \
		{ \
			FuncPtr(*lastFunc)(LastMemberID, Vector<Volt::ShaderParameterMetadata>&, uint32_t); \
			lastFunc = ProcessMember; \
			FuncPtr ptr = (FuncPtr)lastFunc; \
			do \
			{ \
				ptr = reinterpret_cast<MemberFunc>(ptr)(FirstMemberID(), outMetadata, offset); \
			} while (ptr != nullptr); \
		} \
		struct ShaderParameters \
		{ \
			ShaderParameters() \
			{ \
				zzInternal_ProcessMembers(parameterMetadata); \
			} \
			Vector<Volt::ShaderParameterMetadata> parameterMetadata; \
		}; \
		static const Vector<Volt::ShaderParameterMetadata>& GetShaderParameterMetadata() \
		{ \
			static ShaderParameters shaderParameters; \
			return shaderParameters.parameterMetadata; \
		} \
	}; 

#define SHADER_PARAMETER_COMMON_INTERNAL(type, paramName, paramType, resourceAccess) \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t offset) \
	{ \
		auto& paramMetadata = outMetadata.emplace_back(); \
		paramMetadata.name = #paramName; \
		paramMetadata.hashedName = StringHash::Construct(paramMetadata.name); \
		paramMetadata.parameterType = paramType; \
		paramMetadata.structSize = sizeof(type); \
		paramMetadata.structOffset = offset + offsetof(CurrentStruct, paramName); \
		paramMetadata.resourceAccessType = resourceAccess; \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName

#define RG_BUFFER_ACCESS(paramName, access) \
	MemberID##paramName; \
public: \
	Volt::RGBufferRef paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGBufferRef, paramName, Volt::ShaderParameterType::BufferAccess, access)

#define RG_TEXTURE_ACCESS(paramName, access) \
	MemberID##paramName; \
public: \
	Volt::RGTextureRef paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGTextureRef, paramName, Volt::ShaderParameterType::TextureAccess, access)

#define RG_UNIFORM_BUFFER_ACCESS(paramName, access) \
	MemberID##paramName; \
public: \
	Volt::RGUniformBufferRef paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGUniformBufferRef, paramName, Volt::ShaderParameterType::UniformBufferAccess, access)

#define RG_RENDER_TARGETS() \
	MemberIDrenderTargets; \
public: \
	Volt::ShaderParameterRenderTargetBindings renderTargets; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::ShaderParameterRenderTargetBindings, renderTargets, Volt::ShaderParameterType::RenderTargets, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(type, paramName, Volt::ShaderParameterType::Parameter, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_SAMPLER(paramName) \
	MemberID##paramName; \
public: \
	RefPtr<Volt::RHI::SamplerState> paramName; \
	SHADER_PARAMETER_COMMON_INTERNAL(RefPtr<Volt::RHI::SamplerState>, paramName, Volt::ShaderParameterType::Sampler, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_BUFFER_SRV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGBufferSRVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGBufferSRVRef, paramName, Volt::ShaderParameterType::BufferSRV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_BUFFER_UAV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGBufferUAVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGBufferUAVRef, paramName, Volt::ShaderParameterType::BufferUAV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_TEXTURE_SRV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGTextureSRVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGTextureSRVRef, paramName, Volt::ShaderParameterType::TextureSRV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_TEXTURE_UAV(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGTextureUAVRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGTextureUAVRef, paramName, Volt::ShaderParameterType::TextureUAV, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_UNIFORM_BUFFER(type, paramName) \
	MemberID##paramName; \
public: \
	Volt::RGUniformBufferRef paramName = nullptr; \
	SHADER_PARAMETER_COMMON_INTERNAL(Volt::RGUniformBufferRef, paramName, Volt::ShaderParameterType::UniformBuffer, Volt::RGResourceAccess::None)

#define SHADER_PARAMETER_STRUCT(type, paramName) \
	MemberID##paramName; \
public: \
	type paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t offset) \
	{ \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t); \
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
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::ShaderParameterMetadata>& outMetadata, uint32_t offset) \
	{ \
		type::zzInternal_ProcessMembers(outMetadata, offsetof(CurrentStruct, paramName)); \
		FuncPtr(*prevFunc)(MemberID##paramName, Vector<Volt::ShaderParameterMetadata>&, uint32_t); \
		prevFunc = ProcessMember; \
		return (FuncPtr)prevFunc; \
	} \
	typedef NextMemberID##paramName
