#pragma once

#include <CoreUtilities/Archive/Archive.h>
#include <CoreUtilities/Math/Hash.h>

#include <string>


namespace Volt::RHI
{
	enum class InputUsage : uint8_t
	{
		PerVertex,
		PerInstance
	};

	enum class ElementType : uint8_t
	{
		Invalid = 0,
		
		Bool,

		Byte,
		Byte2,
		Byte3,
		Byte4,

		Half,
		Half2,
		Half3,
		Half4,

		UShort,
		UShort2,
		UShort3,
		UShort4,

		Int,
		Int2,
		Int3,
		Int4,

		UInt,
		UInt2,
		UInt3,
		UInt4,

		Float,
		Float2,
		Float3,
		Float4,
	};

	struct BufferElement
	{
		BufferElement() = default;
		BufferElement(ElementType aElementType, const String& aName, uint32_t aArrayIndex = 0, InputUsage aUsage = InputUsage::PerVertex, uint32_t aInputSlot = 0)
			: name(aName), size(GetSizeFromType(aElementType)), arrayIndex(aArrayIndex), inputSlot(aInputSlot), type(aElementType), usage(aUsage)
		{
			CalculateHash();
		}

		static uint32_t GetSizeFromType(ElementType type)
		{
			switch (type)
			{
				case ElementType::Bool: return 4; // HLSL bool size

				case ElementType::Byte: return 1;
				case ElementType::Byte2: return 2;
				case ElementType::Byte3: return 3;
				case ElementType::Byte4: return 4;

				case ElementType::Half: return 2;
				case ElementType::Half2: return 2 * 2;
				case ElementType::Half3: return 2 * 3;
				case ElementType::Half4: return 2 * 4;

				case ElementType::UShort: return 2;
				case ElementType::UShort2: return 2 * 2;
				case ElementType::UShort3: return 2 * 3;
				case ElementType::UShort4: return 2 * 4;

				case ElementType::Int: return 4;
				case ElementType::Int2: return 4 * 2;
				case ElementType::Int3: return 4 * 3;
				case ElementType::Int4: return 4 * 4;

				case ElementType::UInt: return 4;
				case ElementType::UInt2: return 4 * 2;
				case ElementType::UInt3: return 4 * 3;
				case ElementType::UInt4: return 4 * 4;

				case ElementType::Float: return 4;
				case ElementType::Float2: return 4 * 2;
				case ElementType::Float3: return 4 * 3;
				case ElementType::Float4: return 4 * 4;
			}

			return 0;
		}

		uint32_t GetComponentCount(ElementType elementType)
		{
			switch (elementType)
			{
				case ElementType::Bool: return 1;

				case ElementType::Byte: return 1;
				case ElementType::Byte2: return 2;
				case ElementType::Byte3: return 3;
				case ElementType::Byte4: return 4;

				case ElementType::Half: return 1;
				case ElementType::Half2: return 2;
				case ElementType::Half3: return 3;
				case ElementType::Half4: return 4;

				case ElementType::UShort: return 1;
				case ElementType::UShort2: return 2;
				case ElementType::UShort3: return 3;
				case ElementType::UShort4: return 4;

				case ElementType::Int: return 1;
				case ElementType::Int2: return 2;
				case ElementType::Int3: return 3;
				case ElementType::Int4: return 4;

				case ElementType::UInt: return 1;
				case ElementType::UInt2: return 2;
				case ElementType::UInt3: return 3;
				case ElementType::UInt4: return 4;

				case ElementType::Float: return 1;
				case ElementType::Float2: return 2;
				case ElementType::Float3: return 3;
				case ElementType::Float4: return 4;
			}

			return 0;
		}

		friend Archive& operator<<(Archive& archive, BufferElement& value)
		{
			archive << value.name;
			archive << value.offset;
			archive << value.size;
			archive << value.arrayIndex;
			archive << value.inputSlot;
			archive << value.type;
			archive << value.usage;

			if (archive.IsLoading())
			{
				value.hash = value.CalculateHash();
			}

			return archive;
		}

		String name;
		size_t offset;
		uint64_t hash;

		uint32_t size;
		uint32_t arrayIndex;
		uint32_t inputSlot;

		ElementType type;
		InputUsage usage;
	
	private:
		uint64_t CalculateHash()
		{
			uint64_t resultHash = Math::HashCombine(std::hash<String>()(name), std::hash<size_t>()(offset));
			resultHash = Math::HashCombine(resultHash, std::hash<uint32_t>()(size));
			resultHash = Math::HashCombine(resultHash, std::hash<uint32_t>()(arrayIndex));
			resultHash = Math::HashCombine(resultHash, std::hash<uint32_t>()(inputSlot));
			resultHash = Math::HashCombine(resultHash, std::hash<std::underlying_type_t<ElementType>>()(std::to_underlying(type)));
			resultHash = Math::HashCombine(resultHash, std::hash<std::underlying_type_t<InputUsage>>()(std::to_underlying(usage)));

			return resultHash;
		}
	};

	class BufferLayout
	{
	public:
		BufferLayout()
			: m_stride(0)
		{
		}

		BufferLayout(std::initializer_list<BufferElement> aElements)
			: m_elements(aElements), m_stride(0)
		{
			CalculateOffsetAndStride();
		}

		BufferLayout(Vector<BufferElement> aElements)
			: m_elements(aElements), m_stride(0)
		{
			CalculateOffsetAndStride();
		}

		VT_NODISCARD VT_INLINE static String GetNameFromElementType(ElementType type)
		{
			switch (type)
			{
				case ElementType::Bool: return "Bool";

				case ElementType::Byte: return "Byte";
				case ElementType::Byte2: return "Byte2";
				case ElementType::Byte3: return "Byte3";
				case ElementType::Byte4: return "Byte4";

				case ElementType::Half: return "Half";
				case ElementType::Half2: return "Half2";
				case ElementType::Half3: return "Half3";
				case ElementType::Half4: return "Half4";

				case ElementType::UShort: return "UShort";
				case ElementType::UShort2: return "UShort2";
				case ElementType::UShort3: return "UShort3";
				case ElementType::UShort4: return "UShort4";

				case ElementType::Int: return "Int";
				case ElementType::Int2: return "Int2";
				case ElementType::Int3: return "Int3";
				case ElementType::Int4: return "Int4";

				case ElementType::UInt: return "UInt";
				case ElementType::UInt2: return "UInt2";
				case ElementType::UInt3: return "UInt3";
				case ElementType::UInt4: return "UInt4";

				case ElementType::Float: return "Float";
				case ElementType::Float2: return "Float2";
				case ElementType::Float3: return "Float3";
				case ElementType::Float4: return "Float4";
			}

			return "None";
		}

		VT_NODISCARD VT_INLINE uint32_t GetStride() const { return m_stride; }
		VT_NODISCARD VT_INLINE bool IsValid() const { return !m_elements.empty(); }
		VT_NODISCARD VT_INLINE uint64_t GetHash() const { return m_hash; }
		VT_NODISCARD VT_INLINE const Vector<BufferElement>& GetElements() const { return m_elements; }


		friend Archive& operator<<(Archive& archive, BufferLayout& value)
		{
			archive << value.m_elements;
			archive << value.m_stride;
			return archive;
		}

	private:
		void CalculateOffsetAndStride()
		{
			size_t offset = 0;
			uint32_t lastInputSlot = 0;
			m_stride = 0;
			m_hash = 0;

			for (auto& element : m_elements)
			{
				if (lastInputSlot != element.inputSlot)
				{
					lastInputSlot = element.inputSlot;
					offset = 0;
				}
				element.offset = offset;
				offset += element.size;
				m_stride += element.size;

				m_hash = Math::HashCombine(m_hash, element.hash);
			}
		}

		Vector<BufferElement> m_elements;
		uint32_t m_stride = 0;
		uint64_t m_hash = 0;
	};

	using BufferLayoutMap = Map<uint32_t, BufferLayout>;
}
