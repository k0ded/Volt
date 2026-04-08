#pragma once

#include "CoreUtilities/CompilerTraits.h"

// Note: Destroy must be explicitly be called.
class DestructorHelper
{
public:
	DestructorHelper() = default;
	~DestructorHelper() = default;

	template<typename ClassType>
	static DestructorHelper Create(void* dataPtr)
	{
		constexpr auto destructor = [](void* ptr)
		{
			std::launder(reinterpret_cast<ClassType*>(ptr))->~ClassType();
		};

		return DestructorHelper(destructor, dataPtr);
	}

	void Destroy()
	{
		VT_ENSURE(IsValid());
		m_destructorFunc(m_dataPointer);

		Reset();
	}

	void Reset()
	{
		m_destructorFunc = nullptr;
		m_dataPointer = nullptr;
	}

	VT_NODISCARD VT_INLINE bool IsValid() const
	{
		return m_dataPointer != nullptr && m_destructorFunc != nullptr;
	}

private:
	typedef void(*DestructorFunc)(void*);
	
	DestructorHelper(DestructorFunc destructorFunc, void* dataPointer)
		: m_dataPointer(dataPointer),
		m_destructorFunc(destructorFunc)
	{ }

	void* m_dataPointer = nullptr;
	DestructorFunc m_destructorFunc = nullptr;
};
