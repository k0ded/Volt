#pragma once

#include "CoreUtilities/Allocators/ArenaAllocator.h"

template<typename Type, size_t PageSize>
class PagedArenaAllocator
{
public:
	template<typename... Args>
	Type* Allocate(Args&&... args)
	{
		for (auto& page : m_pages)
		{
			if (page.HasAvailableSlots())
			{
				return page.Allocate(std::forward<Args>(args)...);
			}
		}

		auto& newPage = m_pages.emplace_back();
		newPage.AllocateArena(PageSize);

		return newPage.Allocate(std::forward<Args>(args)...);
	}

	void Free(Type* allocation)
	{
		for (auto it = m_pages.begin(); it != m_pages.end(); ++it)
		{
			if ((*it).IsPointerWithinArena(allocation))
			{
				(*it).Free(allocation);
			
				if ((*it).IsEmpty())
				{
					m_pages.erase(it);
				}

				break;
			}
		}
	}

private:
	Vector<ArenaAllocator<Type>> m_pages;
};
