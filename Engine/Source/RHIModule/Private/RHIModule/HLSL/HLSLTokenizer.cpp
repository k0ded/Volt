#include "rhipch.h"

#include "RHIModule/HLSL/HLSLTokenizer.h"

#include <CoreUtilities/Allocators/GlobalMemoryStack.h>
#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <unordered_set>
#include <ranges>

namespace Volt::RHI
{
	static const std::unordered_set<StringView> g_hlslBuiltinResourceTypes =
	{
		"Buffer",
		"RWBuffer",

		"StructuredBuffer",
		"RWStructuredBuffer",

		"ByteAddressBuffer",
		"RWByteAddressBuffer",

		"Texture1D",
		"RWTexture1D",

		"Texture2D",
		"RWTexture2D",

		"Texture3D",
		"RWTexture3D",

		"TextureCube",

		"SamplerState",
		"ConstantBuffer",
		"RaytracingAccelerationStructure"
	};

	static const std::unordered_set<StringView> g_hlslBuiltinResourceTypesTyped =
	{
		"Buffer",
		"RWBuffer",
		"StructuredBuffer",
		"RWStructuredBuffer",

		"ConstantBuffer"
	};

	static const std::unordered_set<StringView> g_hlslBuiltinResourceTypesOptionallyTyped =
	{
		"Texture1D",
		"RWTexture1D",

		"Texture2D",
		"RWTexture2D",

		"Texture3D",
		"RWTexture3D",

		"TextureCube",
	};

	bool IsBlankSpace(char value)
	{
		return value == ' ';
	}

	bool IsBuiltinResourceType(StringView value)
	{
		return g_hlslBuiltinResourceTypes.contains(value);
	}

	bool IsTypedResource(StringView value)
	{
		return g_hlslBuiltinResourceTypesTyped.contains(value);
	}

	bool IsOptionallyTypedResource(StringView value)
	{
		return g_hlslBuiltinResourceTypesOptionallyTyped.contains(value);
	}

	bool IsPrimitiveType(StringView value)
	{
		return
			value.starts_with("bool") ||
			value.starts_with("int") ||
			value.starts_with("uint") ||
			value.starts_with("dword") ||
			value.starts_with("half") ||
			value.starts_with("float") ||
			value.starts_with("double") ||
			value.starts_with("min16float") ||
			value.starts_with("min10float") ||
			value.starts_with("min12int") ||
			value.starts_with("min16uint") ||
			value.starts_with("uint64_t") ||
			value.starts_with("int64_t") ||
			value.starts_with("uint32_t") ||
			value.starts_with("int32_t") ||
			value.starts_with("uint16_t") ||
			value.starts_with("int16_t") ||
			value.starts_with("float16_t");
	}

	HLSLTokenType TokenTypeFromTokenString(StringView value)
	{
		if (value == ">") return HLSLTokenType::GreaterThan;
		if (value == "<") return HLSLTokenType::LessThan;
		if (value == "(") return HLSLTokenType::ParenthasisOpen;
		if (value == ")") return HLSLTokenType::ParenthasisClose;
		if (value == "[") return HLSLTokenType::SqrBracketOpen;
		if (value == "]") return HLSLTokenType::SqrBracketClose;
		if (value == ",") return HLSLTokenType::Comma;
		if (value == ":") return HLSLTokenType::Colon;
		if (value == ".") return HLSLTokenType::Dot;
		if (value == "\"") return HLSLTokenType::Quote;
		if (value == "-") return HLSLTokenType::Minus;
		if (value == "+") return HLSLTokenType::Plus;
		if (value == "*") return HLSLTokenType::Star;
		if (value == "/") return HLSLTokenType::FSlash;
		if (value == "&") return HLSLTokenType::Ampersand;
		if (value == "%") return HLSLTokenType::Percent;
		if (value == "#") return HLSLTokenType::Hashtag;
		if (value == "\n") return HLSLTokenType::Newline;
	
		if (value == "struct") return HLSLTokenType::StructDecl;
		if (value == "enum") return HLSLTokenType::EnumDecl;
		if (value == "typedef") return HLSLTokenType::TypedefDecl;
		if (value == "template") return HLSLTokenType::TemplateDecl;
		if (value == "typename") return HLSLTokenType::TypenameDecl;
		if (value == "register") return HLSLTokenType::RegisterDecl;
		if (value == "const") return HLSLTokenType::ConstDecl;
		if (value == "row_major") return HLSLTokenType::RowMajorDecl;
		if (value == "column_major") return HLSLTokenType::ColumnMajorDecl;
		if (value == "unorm") return HLSLTokenType::UnormDecl;
		if (value == "snorm") return HLSLTokenType::SnormDecl;

		if (value == "extern") return HLSLTokenType::ExternDecl;
		if (value == "nointerpolation") return HLSLTokenType::NointerpolationDecl;
		if (value == "precise") return HLSLTokenType::PreciseDecl;
		if (value == "shared") return HLSLTokenType::SharedDecl;
		if (value == "groupshared") return HLSLTokenType::GroupsharedDecl;
		if (value == "static") return HLSLTokenType::StaticDecl;
		if (value == "uniform") return HLSLTokenType::UniformDecl;
		if (value == "volatile") return HLSLTokenType::VolatileDecl;
		if (value == "linear") return HLSLTokenType::LinearDecl;
		if (value == "centroid") return HLSLTokenType::CentroidDecl;
		if (value == "noperspective") return HLSLTokenType::NoperspectiveDecl;
		if (value == "globallycoherent") return HLSLTokenType::GloballycoherentDecl;

		if (value == "{") return HLSLTokenType::ScopeBegin;
		if (value == "}") return HLSLTokenType::ScopeEnd;

		if (value == ";") return HLSLTokenType::StatementEnd;

		if (IsPrimitiveType(value)) return HLSLTokenType::PrimitiveType;
		if (IsBuiltinResourceType(value)) return HLSLTokenType::BuiltinResourceType;

		return HLSLTokenType::Unknown;
	}

	bool IsSingleCharacterTokenType(HLSLTokenType tokenType)
	{
		switch (tokenType)
		{
			case HLSLTokenType::GreaterThan:
			case HLSLTokenType::LessThan:
			case HLSLTokenType::ParenthasisOpen:
			case HLSLTokenType::ParenthasisClose:
			case HLSLTokenType::SqrBracketOpen:
			case HLSLTokenType::SqrBracketClose:
			case HLSLTokenType::Comma:
			case HLSLTokenType::Colon:
			case HLSLTokenType::Dot:
			case HLSLTokenType::Quote:
			case HLSLTokenType::Minus:
			case HLSLTokenType::Plus:
			case HLSLTokenType::Star:
			case HLSLTokenType::FSlash:
			case HLSLTokenType::Ampersand:
			case HLSLTokenType::Percent:
			case HLSLTokenType::Hashtag:
			case HLSLTokenType::Newline:
			case HLSLTokenType::ScopeBegin:
			case HLSLTokenType::ScopeEnd:
			case HLSLTokenType::StatementEnd:
				return true;
		}

		return false;
	}

	ShaderResourceType GetResourceTypeFromToken(StringView token)
	{
		if (token == "Buffer" || token == "RWBuffer") return ShaderResourceType::TexelBuffer;
		if (token == "StructuredBuffer" || token == "RWStructuredBuffer") return ShaderResourceType::StructuredBuffer;
		if (token == "ByteAddressBuffer" || token == "RWByteAddressBuffer") return ShaderResourceType::StructuredBuffer; //  ShaderResourceType::ByteAddressBuffer
		if (token == "Texture1D" || token == "RWTexture1D") return ShaderResourceType::Texture;
		if (token == "Texture2D" || token == "RWTexture2D") return ShaderResourceType::Texture;
		if (token == "Texture3D" || token == "RWTexture3D") return ShaderResourceType::Texture;
		if (token == "TextureCube") return ShaderResourceType::Texture;
		if (token == "SamplerState") return ShaderResourceType::Sampler;
		if (token == "ConstantBuffer") return ShaderResourceType::UniformBuffer;
		if (token == "RaytracingAccelerationStructure") return ShaderResourceType::AccelerationStructure;

		VT_ENSURE_NO_ENTRY();
		return ShaderResourceType::Sampler;
	}

	ShaderRegisterType GetRegisterTypeFromToken(StringView token)
	{
		if (token == "Buffer" ||
			token == "StructuredBuffer" ||
			token == "ByteAddressBuffer" ||
			token == "RaytracingAccelerationStructure" ||
			token.starts_with("Texture"))
		{
			return ShaderRegisterType::SRV;
		}

		if (token == "ConstantBuffer")
		{
			return ShaderRegisterType::CBV;
		}

		if (token == "SamplerState")
		{
			return ShaderRegisterType::Sampler;
		}

		return ShaderRegisterType::UAV;
	}

	bool IsSingleCharacterTokenType(char token)
	{
		const HLSLTokenType tokenType = TokenTypeFromTokenString(StringView(&token, 1));
		return IsSingleCharacterTokenType(tokenType);
	}

	Vector<HLSLToken> HLSLTokenizer::Tokenize(StringView string)
	{
		VT_PROFILE_FUNCTION();

		m_currentString = string;
		m_currentIndex = 0;

		String tempBuffer;

		Vector<HLSLToken> tokens;

		while (Peek().HasValue())
		{
			// Skip blank spaces.
			if (IsBlankSpace(Peek().Get()))
			{
				Consume();
				continue;
			}
			else if (TryTokenizeSingleCharacterToken(tokens))
			{
				continue;
			}
			else
			{
				size_t begin = m_currentIndex;

				while (Peek().HasValue() && !IsSingleCharacterTokenType(Peek().Get()) && !IsBlankSpace(Peek().Get()))
				{
					tempBuffer.push_back(Consume());
				}

				size_t end = m_currentIndex;

				HLSLToken& newToken = tokens.emplace_back();
				newToken.begin = begin;
				newToken.end = end;
				newToken.value = tempBuffer;
				newToken.tokenType = TokenTypeFromTokenString(tempBuffer);

				tempBuffer.clear();
			}
		}

		return tokens;
	}

	Optional<char> HLSLTokenizer::Peek(size_t offset) const
	{
		if (m_currentIndex + offset >= m_currentString.size())
		{
			return {};
		}

		return m_currentString[m_currentIndex + offset];
	}
	
	char HLSLTokenizer::Consume()
	{
		return m_currentString[m_currentIndex++];
	}

	bool HLSLTokenizer::TryTokenizeSingleCharacterToken(Vector<HLSLToken>& tokens)
	{
		char tokenChar = Peek().Get();
		const HLSLTokenType tokenType = TokenTypeFromTokenString(StringView(&tokenChar, 1));

		if (IsSingleCharacterTokenType(tokenType))
		{
			HLSLToken& newToken = tokens.emplace_back();
			newToken.tokenType = tokenType;
			newToken.begin = m_currentIndex;
			newToken.end = m_currentIndex + 1;
			newToken.value = Consume();

			return true;
		}

		return false;
	}

	Vector<HLSLNode> HLSLParser::Parse(Vector<HLSLToken>&& tokens)
	{
		VT_PROFILE_FUNCTION();

		GlobalMemoryStackMark memMark;

		m_nodes.clear();
		m_tokens = std::move(tokens);
		m_currentIndex = 0;

		int32_t scopeDepth = 0;

		while (Peek() != nullptr)
		{
			if (IsFunctionDeclaration())
			{
				ParseFunctionDeclaration();
				continue;
			}

			switch (Peek()->tokenType)
			{
				case HLSLTokenType::ScopeBegin: scopeDepth++; Consume(); break;
				case HLSLTokenType::ScopeEnd: scopeDepth--; Consume(); break;

				case HLSLTokenType::GloballycoherentDecl:
				case HLSLTokenType::BuiltinResourceType:
				{
					ParseResourceDeclaration(scopeDepth);
					break;
				}

				default:
					Consume();
			}
		}

		return m_nodes;
	}

	const HLSLToken* HLSLParser::Peek(size_t offset) const
	{
		if (m_currentIndex + offset >= m_tokens.size())
		{
			return nullptr;
		}

		return &m_tokens[m_currentIndex + offset];
	}

	const HLSLToken* HLSLParser::Consume()
	{
		return &m_tokens[m_currentIndex++];
	}

	void HLSLParser::ParseResourceDeclaration(int32_t scopeDepth)
	{
		VT_PROFILE_FUNCTION();

		// Find the end of the statement
		GlobalMemoryStackVector<const HLSLToken*> statementTokens;

		while (Peek() != nullptr && Peek()->tokenType != HLSLTokenType::StatementEnd)
		{
			statementTokens.emplace_back(Consume());
		}
		// Consume the statement end.
		statementTokens.emplace_back(Consume());

		// For now, skip any declarations inside a scope.
		if (scopeDepth > 0)
		{
			return;
		}

		HLSLNode& newNode = m_nodes.emplace_back();
		HLSLNode_ResourceDeclaration& newDecl = newNode.data.Emplace<HLSLNode_ResourceDeclaration>();

		size_t tokenIndex = 0;
		newDecl.begin = statementTokens[tokenIndex]->begin;

		if (statementTokens[tokenIndex]->tokenType == HLSLTokenType::GloballycoherentDecl)
		{
			newDecl.isGloballyCoherent = true;
			tokenIndex++;
		}

		newDecl.resourceType = statementTokens[tokenIndex++]->value;
		newDecl.scopeDepth = scopeDepth;

		const bool hasType = IsTypedResource(newDecl.resourceType) ||
			(IsOptionallyTypedResource(newDecl.resourceType) && statementTokens[tokenIndex]->tokenType == HLSLTokenType::LessThan);
	
		if (hasType)
		{
			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::LessThan);
			tokenIndex++;

			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::PrimitiveType || statementTokens[tokenIndex]->tokenType == HLSLTokenType::Unknown);
			newDecl.type = statementTokens[tokenIndex++]->value;

			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::GreaterThan);
			tokenIndex++;
		}

		newDecl.name = statementTokens[tokenIndex++]->value;

		if (statementTokens[tokenIndex]->tokenType == HLSLTokenType::SqrBracketOpen)
		{
			newDecl.isArray = true;

			tokenIndex++;

			if (statementTokens[tokenIndex]->tokenType != HLSLTokenType::SqrBracketClose)
			{
				tokenIndex++;
				VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::Unknown);

				tokenIndex++;
				VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::SqrBracketClose);
			}

			tokenIndex++;
		}

		if (statementTokens[tokenIndex]->tokenType == HLSLTokenType::Colon)
		{
			tokenIndex++;
			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::RegisterDecl);
			tokenIndex++;
			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::ParenthasisOpen);
			tokenIndex++;

			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::Unknown);
			newDecl.binding = statementTokens[tokenIndex++]->value;

			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::Comma);
			tokenIndex++;

			newDecl.space = statementTokens[tokenIndex++]->value;

			VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::ParenthasisClose);
			tokenIndex++;
		}

		VT_ASSERT(statementTokens[tokenIndex]->tokenType == HLSLTokenType::StatementEnd);
		newDecl.end = statementTokens[tokenIndex]->end;
	}

	bool HLSLParser::IsFunctionDeclaration()
	{
		const bool hasReturnType = Peek() != nullptr &&
			(Peek()->tokenType == HLSLTokenType::PrimitiveType ||
			Peek()->tokenType == HLSLTokenType::BuiltinResourceType ||
			Peek()->tokenType == HLSLTokenType::Unknown); // User type

		const bool hasIdentifier = Peek(1) != nullptr &&
			Peek(1)->tokenType == HLSLTokenType::Unknown;

		const bool hasOpenParenthesis = Peek(2) != nullptr &&
			Peek(2)->tokenType == HLSLTokenType::ParenthasisOpen;

		return hasReturnType && hasIdentifier && hasOpenParenthesis;
	}

	void HLSLParser::ParseFunctionDeclaration()
	{
		HLSLNode& newNode = m_nodes.emplace_back();
		HLSLNode_FunctionDeclaration& funcDecl = newNode.data.Emplace<HLSLNode_FunctionDeclaration>();

		// Return type
		Consume();

		// Function name
		funcDecl.name = Consume()->value;
	
		// Consume parameter list
		while (Peek() && Peek()->tokenType != HLSLTokenType::ParenthasisClose)
		{
			Consume();
		}
		Consume();

		while (Peek() && Peek()->tokenType != HLSLTokenType::ScopeBegin)
		{
			Consume();
		}

		funcDecl.bodyBegin = Peek()->begin;
		int32_t depth = 0;

		while (Peek() != nullptr)
		{
			const HLSLToken* token = Consume();
			if (token->tokenType == HLSLTokenType::ScopeBegin)
			{
				depth++;
			}
			else if (token->tokenType == HLSLTokenType::ScopeEnd)
			{
				depth--;
				if (depth == 0)
				{
					// End of function.
					funcDecl.bodyEnd = token->end;
					break;
				}
			}
			else if (token->tokenType == HLSLTokenType::Unknown)
			{
				funcDecl.referencedIdentifiers.emplace_back(token->value);
			}
		}
	}

	void HLSLBindlessRewriter::Rewrite(Vector<HLSLNode>&& nodes, StringView entryPoint, String& string, Vector<ShaderResourceBinding>& outRewrittenResourceBindings)
	{
		VT_PROFILE_FUNCTION();

		constexpr const char* format =
			"uint BindlessIndex_{};\n"
			"typedef {} SafeType{};\n"
			"SafeType{} GetBindlessResource{}() {{ return {}[BindlessIndex_{}]; }}\n"
			"static const SafeType{} {} = GetBindlessResource{}();\n";

		GlobalMemoryStackMark memMark;

		GlobalMemoryStackVector<const HLSLNode_ResourceDeclaration*> resourceDecls;
		GlobalMemoryStackVector<const HLSLNode_FunctionDeclaration*> functionDecls;

		for (const HLSLNode& node : nodes)
		{
			if (node.data.Is<HLSLNode_ResourceDeclaration>())
			{
				resourceDecls.push_back(&node.data.Get<HLSLNode_ResourceDeclaration>());
			}
			else if (node.data.Is<HLSLNode_FunctionDeclaration>())
			{
				functionDecls.push_back(&node.data.Get<HLSLNode_FunctionDeclaration>());
			}
		}

		// Build call graph
		Map<StringView, const HLSLNode_FunctionDeclaration*> nameToFuncDecl;
		for (const HLSLNode_FunctionDeclaration* funcDecl : functionDecls)
		{
			nameToFuncDecl[funcDecl->name] = funcDecl;
		}

		std::unordered_set<StringView> calledFunctions;
		for (const HLSLNode_FunctionDeclaration* funcDecl : functionDecls)
		{
			for (const String& identifier : funcDecl->referencedIdentifiers)
			{
				if (nameToFuncDecl.contains(identifier))
				{
					calledFunctions.insert(identifier);
				}
			}
		}

		std::unordered_set<StringView> reachableFuncs;
		GlobalMemoryStackVector<StringView> workList;

		// Start at the shader entry point
		workList.emplace_back(entryPoint);

		// BFS over call graph
		while (!workList.empty())
		{
			StringView currentFuncName = workList.back();
			workList.pop_back();

			if (reachableFuncs.contains(currentFuncName))
			{
				continue;
			}

			reachableFuncs.insert(currentFuncName);

			if (auto it = nameToFuncDecl.find(currentFuncName); it != nameToFuncDecl.end())
			{
				for (const String& identifier : it->second->referencedIdentifiers)
				{
					if (nameToFuncDecl.contains(identifier))
					{
						workList.push_back(identifier);
					}
				}
			}
		}

		// Find all reachable identifiers
		std::unordered_set<StringView> usedIdentifiers;
		for (const StringView& funcName : reachableFuncs)
		{
			if (auto it = nameToFuncDecl.find(funcName); it != nameToFuncDecl.end())
			{
				for (const String& identifier : it->second->referencedIdentifiers)
				{
					usedIdentifiers.insert(identifier);
				}
			}
		}

		for (const HLSLNode_ResourceDeclaration* resourceDecl : std::ranges::reverse_view(resourceDecls))
		{
			if (resourceDecl->isArray ||
				resourceDecl->scopeDepth > 0 ||
				!resourceDecl->space.empty() ||
				resourceDecl->resourceType == "ConstantBuffer")
			{
				continue;
			}

			String resourceType;
			if (!resourceDecl->type.empty())
			{
				resourceType = FormatString("{}<{}>", resourceDecl->resourceType, resourceDecl->type);
			}
			else
			{
				resourceType = resourceDecl->resourceType;
			}

			String heapName = "ResourceDescriptorHeap";
			if (resourceDecl->resourceType == "SamplerState")
			{
				heapName = "SamplerDescriptorHeap";
			}

			const String newResourceDecl = FormatString(format, resourceDecl->name, resourceType, 
				resourceDecl->name, resourceDecl->name, resourceDecl->name, 
				heapName, resourceDecl->name, resourceDecl->name, 
				resourceDecl->name, resourceDecl->name);
			
			string.replace(resourceDecl->begin, resourceDecl->end - resourceDecl->begin, newResourceDecl);


			if (usedIdentifiers.contains(resourceDecl->name))
			{
				ShaderResourceBinding& rewrittenBinding = outRewrittenResourceBindings.emplace_back();
				rewrittenBinding.name = resourceDecl->name;
				rewrittenBinding.resourceType = GetResourceTypeFromToken(resourceDecl->resourceType);
				rewrittenBinding.registerType = GetRegisterTypeFromToken(resourceDecl->resourceType);
			}
		}
	}
}
