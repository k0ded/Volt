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

	template<typename T>
	struct ShaderPermutationTraits;

	template<typename T>
	struct ShaderPermutationTraits
	{
		static_assert(!std::is_same_v<T, T>, "ShaderPermutationTraits not specialized for this type");
	};

	template<>
	struct ShaderPermutationTraits<bool>
	{
		static constexpr size_t DomainSize = 2;

		static size_t ToIndex(bool value)
		{
			return value ? 1 : 0;
		}

		static bool FromIndex(size_t index)
		{
			return index != 0;
		}

		static void Resolve(std::string_view name, bool value, RHI::ShaderPermutationConfig& permutationConfig)
		{
			permutationConfig.AddPermutation(std::string(name), std::to_string(value));
		}
	};

	template<typename T>
		requires std::is_enum_v<T>
	struct ShaderPermutationTraits<T>
	{
		using UnderlyingType = std::underlying_type_t<T>;

		static constexpr size_t DomainSize = static_cast<size_t>(T::Count);

		static size_t ToIndex(T value)
		{
			return static_cast<size_t>(std::to_underlying(value));
		}

		static T FromIndex(size_t index)
		{
			return static_cast<T>(index);
		}

		static void Resolve(std::string_view name, T value, RHI::ShaderPermutationConfig& permutationConfig)
		{
			permutationConfig.AddPermutation(std::string(name), std::to_string(std::to_underlying(value)));
		}
	};


	template<typename T, CompileTimeString Name>
	struct ShaderPermutationDefinition
	{
		using PermutationType = T;
		using Traits = ShaderPermutationTraits<T>;
		inline static constexpr std::string_view PermutationName = Name;

		static constexpr size_t DomainSize()
		{
			return Traits::DomainSize;
		}

		static size_t ToIndex(const Any& value)
		{
			return Traits::ToIndex(value.Cast<T>());
		}

		static void Resolve(const Any& value, RHI::ShaderPermutationConfig& permutationConfig)
		{
			Traits::Resolve(PermutationName, value.Cast<T>(), permutationConfig);
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

		void InitializeWithPermutationIndex(size_t index)
		{
			ForEachIndex<std::tuple_size_v<PermutationTuple>>([this, &index]<size_t I>() 
			{
				using PermutationDefinition = std::tuple_element_t<I, PermutationTuple>;

				constexpr size_t domain = PermutationDefinition::DomainSize();
				const size_t valueIndex = (index / s_permutationStrides[I]) % domain;

				using ValueType = typename PermutationDefinition::PermutationType;
				const ValueType value = ShaderPermutationTraits<ValueType>::FromIndex(valueIndex);

				this->template Set<PermutationDefinition>(value);
			});
		}

		void ResolvePermutations(RHI::ShaderPermutationConfig& permutationConfig) const
		{
			for (const PermutationContainer& permutationContainer : m_permutations)
			{
				permutationContainer.resolvePermutationFunc(permutationContainer.value, permutationConfig);
			}

			permutationConfig.SetPermutationIndex(GetPermutationIndex());
		}

		void Validate() const
		{
			for (const PermutationContainer& permutationContainer : m_permutations)
			{
				VT_ENSURE_MSG(permutationContainer.value.HasValue(), "All permutations must have been set!");
			}
		}

		size_t GetPermutationIndex() const
		{
			Validate();

			size_t index = 0;

			ForEachIndex<std::tuple_size_v<PermutationTuple>>(
			[this, &index]<size_t I>() 
			{
				using PermutationType = std::tuple_element_t<I, PermutationTuple>;
				const size_t valueIndex = PermutationType::ToIndex(m_permutations[I].value);

				index += valueIndex * s_permutationStrides[I];
			});

			return index;
		}

		static constexpr size_t GetTotalPermutationCount()
		{
			size_t count = 1;
			ForEachIndex<std::tuple_size_v<PermutationTuple>>(
			[&]<size_t I>() 
			{
				using PermutationType = std::tuple_element_t<I, PermutationTuple>;
				count *= PermutationType::DomainSize();
			});

			return count;
		}

	private:
		struct PermutationContainer
		{
			Any value;
			std::function<void(const Any&, RHI::ShaderPermutationConfig&)> resolvePermutationFunc;
		};

		template<size_t N, typename F>
		static constexpr void ForEachIndex(F&& func)
		{
			[] <size_t... Is>(std::index_sequence<Is...>, F&& f)
			{
				(f.template operator()<Is>(), ...);
			}(std::make_index_sequence<N>{}, std::forward<F>(func));
		}

		template<size_t I>
		static constexpr size_t ComputePermutationStride()
		{
			size_t stride = 1;

			ForEachIndex<I>([&]<size_t J>() 
			{
				using PermutationType = std::tuple_element_t<J, PermutationTuple>;
				stride *= PermutationType::DomainSize();
			});
			return stride;
		}

		inline static constexpr Array<size_t, std::tuple_size_v<PermutationTuple>> s_permutationStrides =
		[]() 
		{
			Array<size_t, std::tuple_size_v<PermutationTuple>> strides{};
			
			ForEachIndex<std::tuple_size_v<PermutationTuple>>(
			[&]<size_t I>()
			{
				strides[I] = ComputePermutationStride<I>();
			});

			return strides;
		}();

		Array<PermutationContainer, std::tuple_size_v<PermutationTuple>> m_permutations;
	};
}

#define SHADER_PERMUTATION_BOOL(permutationName) public ShaderPermutationDefinition<bool, permutationName> {}
#define SHADER_PERMUTATION_ENUM(permutationName, enumType) public ShaderPermutationDefinition<enumType, permutationName> {}
