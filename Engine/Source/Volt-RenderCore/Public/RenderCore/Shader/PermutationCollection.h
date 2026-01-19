#pragma once

#include <RHIModule/Shader/ShaderPermutationConfig.h>

#include <CoreUtilities/Concepts.h>
#include <CoreUtilities/TupleUtility.h>
#include <CoreUtilities/CompileTimeString.h>
#include <CoreUtilities/Containers/Array.h>
#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Any.h>

namespace Volt
{
	template<typename T>
	concept IsPermutationType = requires
	{
		typename T::PermutationType;
	};

	template<typename... Ts>
	concept IsValidPermutationCollection = (IsPermutationType<Ts> && ...);

	template<typename T, typename Tuple>
	concept IsValidPermutation = ::Utility::TupleTypeIndex<T, Tuple>::IsValid;

	template<typename T, CompileTimeString Name>
	struct ShaderPermutationDefinition
	{
		using PermutationType = T;
		inline static constexpr std::string_view PermutationName = Name;

		static void Resolve(const Any& value, RHI::ShaderPermutationConfig& permutationConfig)
		{
			if constexpr (std::is_same_v<PermutationType, bool>)
			{
				permutationConfig.AddPermutation(std::string(PermutationName), std::to_string(value.Cast<bool>()));
			}
			else if constexpr (std::is_enum_v<PermutationType>)
			{
				permutationConfig.AddPermutation(std::string(PermutationName), std::to_string(std::to_underlying(value.Cast<PermutationType>())));
			}
			else
			{
				static_assert(false, "Not implemented!");
			}
		}

		static size_t Hash(const Any& value)
		{
			if constexpr (std::is_same_v<PermutationType, bool>)
			{
				return std::hash<bool>()(value.Cast<bool>());
			}
			else if constexpr (std::is_enum_v<PermutationType>)
			{
				using UnderlyingType = std::underlying_type_t<PermutationType>;

				return std::hash<UnderlyingType>()(std::to_underlying(value.Cast<PermutationType>()));
			}
			else
			{
				static_assert(false, "Not implemented!");
			}
		}
	};

	template<typename... Ts>
		requires(IsValidPermutationCollection<Ts...>)
	class PermutationCollection
	{
	public:
		using PermutationTuple = std::tuple<Ts...>;

		PermutationCollection() = default;

		template<typename U> 
			requires(IsValidPermutation<U, PermutationTuple>)
		void Set(typename U::PermutationType value)
		{
			using PermutationTraits = ::Utility::TupleTypeIndex<U, PermutationTuple>;

			constexpr size_t PermutationIndex = PermutationTraits::Value;
			m_permutations[PermutationIndex].value.Emplace(value);
			m_permutations[PermutationIndex].resolvePermutationFunc = [](const Any& value, RHI::ShaderPermutationConfig& permutationConfig)
			{
				using PermutationDefinitionType = typename std::tuple_element_t<PermutationIndex, PermutationTuple>;
				PermutationDefinitionType::Resolve(value, permutationConfig);
			};

			m_permutations[PermutationIndex].hashPermutationFunc = [](const Any& value) -> size_t
			{
				using PermutationDefinitionType = typename std::tuple_element_t<PermutationIndex, PermutationTuple>;
				return PermutationDefinitionType::Hash(value);
			};

			m_hash = 0;
		}

		template<typename U>
			requires(IsValidPermutation<U, PermutationTuple>)
		typename U::PermutationType Get() const
		{
			using PermutationTraits = ::Utility::TupleTypeIndex<U, PermutationTuple>;

			constexpr size_t PermutationIndex = PermutationTraits::Value;
			VT_ENSURE_MSG(m_permutations[PermutationIndex].value.HasValue(), "Permutation must have been set!");
			return m_permutations[PermutationIndex].value.Cast<typename U::PermutationType>();
		}

		void ResolvePermutations(RHI::ShaderPermutationConfig& permutationConfig) const
		{
			for (const PermutationContainer& permutationContainer : m_permutations)
			{
				permutationContainer.resolvePermutationFunc(permutationContainer.value, permutationConfig);
			}
		}

		void Validate() const
		{
			for (const PermutationContainer& permutationContainer : m_permutations)
			{
				VT_ENSURE_MSG(permutationContainer.value.HasValue(), "All permutations must have been set!");
			}
		}

		size_t GetHash() const
		{
			if (m_hash != 0)
			{
				return m_hash;
			}

			for (const PermutationContainer& permutationContainer : m_permutations)
			{
				m_hash = Math::HashCombine(m_hash, permutationContainer.hashPermutationFunc(permutationContainer.value));
			}

			return m_hash;
		}

	private:
		struct PermutationContainer
		{
			Any value;
			std::function<void(const Any&, RHI::ShaderPermutationConfig&)> resolvePermutationFunc;
			std::function<size_t(const Any&)> hashPermutationFunc;
		};

		Array<PermutationContainer, std::tuple_size_v<PermutationTuple>> m_permutations;
		mutable size_t m_hash = 0;
	};
}

#define SHADER_PERMUTATION_BOOL(permutationName) public ShaderPermutationDefinition<bool, permutationName> {}
#define SHADER_PERMUTATION_ENUM(permutationName, enumType) public ShaderPermutationDefinition<enumType, permutationName> {}
