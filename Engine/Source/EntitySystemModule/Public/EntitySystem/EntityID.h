#pragma once

#include "EntitySystem/Config.h"

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Archive/Archive.h>

#include <format>

namespace Volt
{
	class VTES_API EntityID
	{
	public:
		EntityID();
		constexpr EntityID(uint32_t uuid)
			: m_uuid(uuid)
		{
		}

		EntityID(const EntityID&) = default;
		~EntityID() = default;

		operator uint32_t() const { return m_uuid; }

		VT_NODISCARD VT_INLINE const uint32_t Get() const { return m_uuid; }

		VT_INLINE friend Archive& operator<<(Archive& archive, EntityID& value)
		{
			archive << value.m_uuid;
			return archive;
		}

		static EntityID Null();

	private:
		uint32_t m_uuid;
	};
}

namespace std
{
	template <typename T> struct hash;

	template<>
	struct hash<Volt::EntityID>
	{
		std::size_t operator()(const Volt::EntityID& uuid) const
		{
			return (uint32_t)uuid;
		}
	};

	template <>
	struct formatter<Volt::EntityID> : formatter<string>
	{
		auto format(Volt::EntityID id, format_context& ctx) const
		{
			return formatter<string>::format(
			  std::format("{}", id.Get()), ctx);
		}
	};
}
