#pragma once

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>

#include <type_traits>

namespace Volt
{
	struct RenderPrimitiveData;

	class MeshPassProcessor
	{
	public:
		virtual ~MeshPassProcessor() = default;

		virtual void AddRenderPrimitive(const RenderPrimitiveData& renderPrimitive) = 0;
		virtual void RemoveRenderPrimitive(UUID64 renderPrimitveId) = 0;

	protected:
		void BuildMeshDrawCommands();

	private:
	};


	class MeshPassProcessorRegistry
	{
	public:
		MeshPassProcessorRegistry();

		template<typename T>
		requires (std::is_base_of_v<T, MeshPassProcessor>)
		void AddProcessor()
		{
			constexpr size_t AllocationSize = sizeof(T);

			void* alloc = m_meshPassProcessorAllocator.Allocate(AllocationSize);
			T* processor = new (alloc) T();

			m_meshPassDestructors.emplace_back() = DestructorHelper::Create<T>(alloc);
			m_meshPassProcessors.emplace_back(processor);
		}

	private:
		LinearAllocator<> m_meshPassProcessorAllocator;
		Vector<MeshPassProcessor*> m_meshPassProcessors;
		Vector<DestructorHelper> m_meshPassDestructors;
	};
}
