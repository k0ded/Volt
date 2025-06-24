#include "rcpch.h"

#include "RenderCore/RenderGraph/RenderGraphBlackboard.h"

namespace Volt
{
	RenderGraphBlackboard::~RenderGraphBlackboard()
	{
		for (auto& [typeIndex, typeInfo] : m_typeInfos)
		{
			typeInfo.typeDestructor(typeInfo.typePtr);
		}
	}

	void* RenderGraphBlackboard::Allocate(size_t size)
	{
		return m_allocator.Allocate(size);
	}
}
