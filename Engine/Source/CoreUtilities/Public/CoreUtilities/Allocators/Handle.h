#pragma once

#include <CoreUtilities/Concepts.h>
#include <CoreUtilities/CompilerTraits.h>

template<typename Type>
class Handle
{
public:
	Handle() = default;

	Handle(Type* pointer)
		: m_pointer(pointer)
	{}

	template<IsDerivedFrom<Type> OtherType>
	Handle(OtherType* pointer)
		: m_pointer(pointer)
	{}

	template<IsDerivedFrom<Type> OtherType>
	Handle(Handle<OtherType> otherHandle)
		: m_pointer(otherHandle.m_pointer)
	{
	}

	~Handle() = default;

	template<typename OtherType>
	VT_NODISCARD VT_INLINE Handle<OtherType> As() const 
	{
		return Handle<OtherType>(reinterpret_cast<OtherType*>(m_pointer));
	}

	VT_NODISCARD VT_INLINE Type* GetRaw() const
	{
		return m_pointer;
	}

	VT_NODISCARD VT_INLINE Type* operator->() { return m_pointer; }
	VT_NODISCARD VT_INLINE Type* operator->() const { return m_pointer; }
	VT_NODISCARD VT_INLINE Type& operator*() { return *m_pointer; }
	VT_NODISCARD VT_INLINE Type& operator*() const { return *m_pointer; }

	VT_INLINE Handle<Type>& operator=(const Handle<Type>& other) { m_pointer = other.m_pointer; return *this; }

	template<IsDerivedFrom<Type> OtherType>
	VT_INLINE Handle<Type>& operator=(const Handle<OtherType>& other) { m_pointer = other.m_pointer; return *this; }

	VT_INLINE Handle<Type>& operator=(Type* other) { m_pointer = other; return *this; }

	template<IsDerivedFrom<Type> OtherType>
	VT_INLINE Handle<Type>& operator=(OtherType* other) { m_pointer = other; return *this; }

	VT_INLINE bool operator==(const Handle<Type>& other) const { return m_pointer == other.m_pointer; }
	VT_INLINE bool operator!() const { return m_pointer == nullptr; }
	VT_INLINE explicit operator bool() const { return m_pointer != nullptr; }

private:
	template<typename U>
	friend class Handle;

	Type* m_pointer = nullptr;
};
