#pragma once

#if __cplusplus
	
#include <RHIModule/Shader/BufferLayout.h>

namespace VertexDeclTypes
{
	using namespace Volt::RHI;

	struct Bool { inline static constexpr ElementType ShaderType = ElementType::Bool; };

	struct Half { inline static constexpr ElementType ShaderType = ElementType::Half; };
	struct Half2 { inline static constexpr ElementType ShaderType = ElementType::Half2; };
	struct Half3 { inline static constexpr ElementType ShaderType = ElementType::Half3; };
	struct Half4 { inline static constexpr ElementType ShaderType = ElementType::Half4; };

	struct UShort { inline static constexpr ElementType ShaderType = ElementType::UShort; };
	struct UShort2 { inline static constexpr ElementType ShaderType = ElementType::UShort2; };
	struct UShort3 { inline static constexpr ElementType ShaderType = ElementType::UShort3; };
	struct UShort4 { inline static constexpr ElementType ShaderType = ElementType::UShort4; };

	struct Int { inline static constexpr ElementType ShaderType = ElementType::Int; };
	struct Int2 { inline static constexpr ElementType ShaderType = ElementType::Int2; };
	struct Int3 { inline static constexpr ElementType ShaderType = ElementType::Int3; };
	struct Int4 { inline static constexpr ElementType ShaderType = ElementType::Int4; };

	struct UInt { inline static constexpr ElementType ShaderType = ElementType::UInt; };
	struct UInt2 { inline static constexpr ElementType ShaderType = ElementType::UInt2; };
	struct UInt3 { inline static constexpr ElementType ShaderType = ElementType::UInt3; };
	struct UInt4 { inline static constexpr ElementType ShaderType = ElementType::UInt4; };

	struct Float { inline static constexpr ElementType ShaderType = ElementType::Float; };
	struct Float2 { inline static constexpr ElementType ShaderType = ElementType::Float2; };
	struct Float3 { inline static constexpr ElementType ShaderType = ElementType::Float3; };
	struct Float4 { inline static constexpr ElementType ShaderType = ElementType::Float4; };
}

#define BEGIN_VERTEX_DECLARATION(structName) \
	struct structName \
	{ \
	private: \
		struct FirstMemberID {}; \
		typedef void* FuncPtr; \
		typedef FuncPtr (*MemberFunc)(FirstMemberID, Vector<Volt::RHI::BufferElement>& bufferLayout); \
		static FuncPtr ProcessMember(FirstMemberID, Vector<Volt::RHI::BufferElement>& bufferLayout)	\
		{ \
			return nullptr; \
		} \
		typedef FirstMemberID

#define END_VERTEX_DECLARATION() \
	LastMemberID; \
	public: \
	static void zzInternal_ProcessMembers(Vector<Volt::RHI::BufferElement>& outBufferLayout) \
	{ \
		FuncPtr(*lastFunc)(LastMemberID, Vector<Volt::RHI::BufferElement>&); \
		lastFunc = ProcessMember; \
		FuncPtr ptr = (FuncPtr)lastFunc; \
		do \
		{ \
			ptr = reinterpret_cast<MemberFunc>(ptr)(FirstMemberID(), outBufferLayout); \
		} while (ptr != nullptr); \
	} \
	static const Volt::RHI::BufferLayout& GetVertexInputLayout() \
	{ \
		static bool isInitialized = false; \
		static Volt::RHI::BufferLayout vertexInputLayout; \
		if (!isInitialized) \
		{ \
			Vector<Volt::RHI::BufferElement> bufferElements; \
			zzInternal_ProcessMembers(bufferElements); \
			vertexInputLayout = Volt::RHI::BufferLayout(bufferElements); \
			isInitialized = true; \
		} \
		return vertexInputLayout; \
	} \
};

#define VERTEX_INPUT(type, paramName, identifier, inputSlot) \
	MemberID##paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::RHI::BufferElement>& outBufferLayout) \
	{ \
		outBufferLayout.emplace_back(VertexDeclTypes::type::ShaderType, #paramName, 0, Volt::RHI::InputUsage::PerVertex, inputSlot); \
	} \
	typedef NextMemberID##paramName

#define VERTEX_INPUT_PER_INSTANCE(type, paramName, identifier, inputSlot) \
	MemberID##paramName; \
private: \
	struct NextMemberID##paramName {}; \
	static FuncPtr ProcessMember(NextMemberID##paramName, Vector<Volt::RHI::BufferElement>& outBufferLayout) \
	{ \
		outBufferLayout.emplace_back(VertexDeclTypes::type::ShaderType, #paramName, 0, Volt::RHI::InputUsage::PerInstance, inputSlot); \
	} \
	typedef NextMemberID##paramName

#define VERTEX_SYSTEM_VALUE(type, paramName, systemValue)

#else

typedef Bool bool;
typedef Half float;
typedef Half2 float2;
typedef Half3 float3;
typedef Half4 float4;
typedef UShort uint;
typedef UShort2 uint2;
typedef UShort3 uint3;
typedef UShort4 uint4;
typedef Int int;
typedef Int2 int2;
typedef Int3 int3;
typedef Int4 int4;
typedef UInt uint;
typedef UInt2 uint2;
typedef UInt3 uint3;
typedef UInt4 uint4;
typedef Float float;
typedef Float2 float2;
typedef Float3 float3;
typedef Float4 float4;

#define BEGIN_VERTEX_DECLARATION(structName) \
	struct structName \
	{ \

#define END_VERTEX_DECLARATION() \
	}; \

#define VERTEX_INPUT(type, paramName, identifier, inputSlot) \
	type paramName : identifier;

#define VERTEX_INPUT_PER_INSTANCE(type, paramName, identifier, inputSlot) \
	VERTEX_INPUT(type, paramName, identifier, inputSlot)

#define VERTEX_SYSTEM_VALUE(type, paramName, systemValue) \
	type paramName : systemValue;

#endif