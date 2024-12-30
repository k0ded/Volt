#pragma once

#include <CoreUtilities/Core.h>

namespace Volt
{
	class PhysicsHandleType
	{
	public:
		virtual ~PhysicsHandleType() = default;
		VT_DELETE_COPY_MOVE(PhysicsHandleType);

		template<typename T>
		constexpr T GetHandle() const
		{
			return reinterpret_cast<T>(GetHandleImpl());
		}

	protected:
		PhysicsHandleType() = default;
		virtual void* GetHandleImpl() const = 0;
	};
}
