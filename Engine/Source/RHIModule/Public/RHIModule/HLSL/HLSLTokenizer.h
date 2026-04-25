#pragma once

#include "RHIModule/Core/Core.h"
#include "RHIModule/Shader/ShaderCommon.h"

#include <CoreUtilities/String/StringView.h>
#include <CoreUtilities/Optional.h>
#include <CoreUtilities/Variant.h>
#include <CoreUtilities/String/VoltString.h>

namespace Volt::RHI
{
	enum class HLSLTokenType : uint8_t
	{
		GreaterThan,
		LessThan,
		ParenthasisOpen,
		ParenthasisClose,
		SqrBracketOpen,
		SqrBracketClose,

		Comma,
		Colon,
		Dot,
		Quote,
		Minus,
		Plus,
		Star,
		FSlash,
		Ampersand,
		Percent,
		Hashtag,
		Newline,

		StructDecl,
		EnumDecl,
		TypedefDecl,
		TemplateDecl,
		TypenameDecl,
		RegisterDecl,

		ConstDecl,
		RowMajorDecl,
		ColumnMajorDecl,
		UnormDecl,
		SnormDecl,

		ExternDecl,
		NointerpolationDecl,
		PreciseDecl,
		SharedDecl,
		GroupsharedDecl,
		StaticDecl,
		UniformDecl,
		VolatileDecl,
		LinearDecl,
		CentroidDecl,
		NoperspectiveDecl,
		GloballycoherentDecl,

		PrimitiveType,
		BuiltinResourceType,

		ScopeBegin,
		ScopeEnd,
		StatementEnd,
		Unknown
	};

	struct HLSLToken
	{
		HLSLTokenType tokenType;
		String value;

		size_t begin;
		size_t end;
	};

	class HLSLTokenizer
	{
	public:
		HLSLTokenizer() = default;

		VTRHI_API Vector<HLSLToken> Tokenize(StringView string);

	private:
		Optional<char> Peek(size_t ahead = 0) const;
		char Consume();

		bool TryTokenizeSingleCharacterToken(Vector<HLSLToken>& tokens);

		StringView m_currentString;
		size_t m_currentIndex = 0;
	};

	struct HLSLNode_ResourceDeclaration
	{
		String resourceType;
		String name;
		String type;
		String binding;
		String space;
	
		size_t begin;
		size_t end;
		int32_t scopeDepth = 0;

		bool isArray : 1 = false;
		bool isGloballyCoherent : 1 = false;
	};

	struct HLSLNode_FunctionDeclaration
	{
		String name;
		size_t bodyBegin;
		size_t bodyEnd;

		Vector<String> referencedIdentifiers;
	};

	struct HLSLNode
	{
		Variant<HLSLNode_ResourceDeclaration, HLSLNode_FunctionDeclaration> data;
	};

	class HLSLParser
	{
	public:
		HLSLParser() = default;

		VTRHI_API Vector<HLSLNode> Parse(Vector<HLSLToken>&& tokens);

	private:
		const HLSLToken* Peek(size_t offset = 0) const;
		const HLSLToken* Consume();

		void ParseResourceDeclaration(int32_t scopeDepth);
		void ParseFunctionDeclaration();

		bool IsFunctionDeclaration();

		Vector<HLSLToken> m_tokens;
		Vector<HLSLNode> m_nodes;
		size_t m_currentIndex = 0;
	};

	class HLSLBindlessRewriter
	{
	public:
		HLSLBindlessRewriter() = default;

		VTRHI_API void Rewrite(Vector<HLSLNode>&& nodes, StringView entryPoint, String& string, Vector<ShaderResourceBinding>& outRewrittenResourceBindings);
	};
}
