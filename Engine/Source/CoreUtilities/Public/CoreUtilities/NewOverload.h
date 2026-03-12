#pragma once

#include "CoreUtilities/Malloc.h"

#include <cstddef>
#include <new>

#define VT_OVERLOAD_NEW_OPERATORS \
	void* operator new		(std::size_t size) { return Memory::Malloc(size); }	\
	void  operator delete	(void* ptr) noexcept { Memory::Free(ptr); }			\
	void* operator new[]	(std::size_t size) { return Memory::Malloc(size); }  \
	void  operator delete[]	(void* ptr) noexcept { Memory::Free(ptr); } \
	\
	void* operator new		(std::size_t size, const std::nothrow_t&) noexcept { return Memory::Malloc(size); } \
	void  operator delete	(void* ptr, const std::nothrow_t&) noexcept { return Memory::Free(ptr); } \
	void* operator new[]	(std::size_t size, const std::nothrow_t&) noexcept { return Memory::Malloc(size); }  \
	void  operator delete[]	(void* ptr, const std::nothrow_t&) noexcept { Memory::Free(ptr); } \
	\
	void* operator new		(std::size_t size, std::align_val_t align) { return Memory::Malloc(size, static_cast<size_t>(align)); }	\
	void  operator delete	(void* ptr, std::align_val_t align) noexcept { Memory::Free(ptr); }			\
	void* operator new[]	(std::size_t size, std::align_val_t align) { return Memory::Malloc(size, static_cast<size_t>(align)); }  \
	void  operator delete[]	(void* ptr, std::align_val_t align) noexcept { Memory::Free(ptr); } \
	\
	void* operator new		(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return Memory::Malloc(size, static_cast<size_t>(align)); }	\
	void  operator delete	(void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { Memory::Free(ptr); }			\
	void* operator new[]	(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept { return Memory::Malloc(size, static_cast<size_t>(align)); }  \
	void  operator delete[]	(void* ptr, std::align_val_t align, const std::nothrow_t&) noexcept { Memory::Free(ptr); } \
	\
	void operator delete	(void* ptr, std::size_t size) noexcept { Memory::Free(ptr); }	\
	void operator delete[]	(void* ptr, std::size_t size) noexcept { Memory::Free(ptr); } \
	void operator delete	(void* ptr, std::size_t size, std::align_val_t align) noexcept { Memory::Free(ptr); } \
	void operator delete[]	(void* ptr, std::size_t size, std::align_val_t align) noexcept { Memory::Free(ptr); }
