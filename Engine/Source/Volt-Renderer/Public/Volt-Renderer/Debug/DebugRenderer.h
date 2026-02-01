#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Debug/DebugVertices.h"

#include "Volt-Renderer/Mesh/Mesh.h"
#include "Volt-Renderer/Material/RenderMaterial.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Allocators/PagedAtomicLinearAllocator.h>
#include <CoreUtilities/Math/TQS.h>

namespace Volt
{
	class RenderGraph;
	struct RenderView;
	struct ShaderParameterRenderTargetBindings;

	class DebugRenderer
	{
	public:
		VTR_API DebugRenderer();

		VTR_API void DrawLine(const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);
		VTR_API void DrawLineSphere(const glm::vec3& center, float radius, const glm::vec4& color);

		VTR_API void DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, uint32_t userData = 0, bool isViewSpacePosition = false);
		VTR_API void DrawBillboard(const glm::vec3& position, const glm::vec3& size, const glm::vec4& color, RefPtr<RHI::Image> texture, uint32_t userData = 0, bool isViewSpacePosition = false);

		VTR_API void DrawMesh(Ref<Mesh> mesh, Ref<RenderMaterial> material, const TQS& transform, uint32_t userData = 0);

		VTR_API void Render(RenderGraph& renderGraph, const RenderView& renderView, RGTextureRef dstTexture, RGTextureRef depthTexture);
		VTR_API void Reset();

		VTR_API size_t GetNumLinesPerLineSphere() const;

		/*
			Custom rendering
		*/
		VTR_API void RenderBillboards(RenderGraph& renderGraph, RefPtr<RHI::Shader> pixelShader, const RenderView& view, const ShaderParameterRenderTargetBindings& renderTargets, bool shouldClear);

	private:
		template<typename T>
		class Allocator
		{
		public:
			using AllocatorType = PagedAtomicLinearAllocator<sizeof(T) * 1024>;
			using IteratorType = AllocatorType::PageIterator;

			~Allocator()
			{
				Release();
			}

			VT_INLINE T* AllocateCount(size_t count)
				requires(std::is_trivial_v<T>)
			{
				return reinterpret_cast<T*>(m_allocator.Allocate(sizeof(T) * count));
			}

			VT_INLINE T* Allocate()
			{
				void* dataPtr = m_allocator.Allocate(sizeof(T));
				new (dataPtr) T();

				return reinterpret_cast<T*>(dataPtr);
			}

			void Release()
			{
				if (!std::is_trivial_v<T>)
				{
					for (IteratorType it(m_allocator); it; ++it)
					{
						const T* dataPtr = reinterpret_cast<T*>(*it);
						const size_t numAllocated = it.Size() / sizeof(T);
						
						for (size_t i = 0; i < numAllocated; ++i)
						{
							dataPtr[i].~T();
						}
 					}
				}

				m_allocator.Reset();
			}

			VT_INLINE size_t GetNumAllocated()
			{
				return GetAllocatedSize() / sizeof(T);
			}

			VT_INLINE size_t GetAllocatedSize()
			{
				size_t totalSize = 0;

				for (IteratorType it(m_allocator); it; ++it)
				{
					totalSize += it.Size();
				}

				return totalSize;
			}

			/*
				Only allowed for trivial types.
			*/
			VT_INLINE void MemcopyInto(T* dstPtr, size_t dstByteSize)
				requires(std::is_trivial_v<T>)
			{
				size_t offset = 0;

				uint8_t* bytePtr = reinterpret_cast<uint8_t*>(dstPtr);

				for (IteratorType it(m_allocator); it; ++it)
				{
					memcpy_s(bytePtr + offset, dstByteSize - offset, *it, it.Size());
					offset += it.Size();
				}
			}

			/*
				Not allowed for trivial types, since that never makes sense (what I can think of).
				This will invoke the copy constructor.
			*/
			VT_INLINE void CopyInto(T* dstPtr, size_t dstSize)
				requires((!std::is_trivial_v<T>))
			{
				size_t index = 0;
				for (IteratorType it(m_allocator); it && index < dstSize; ++it)
				{
					const size_t numInPage = it.Size() / sizeof(T);
					const T* pageDataPtr = reinterpret_cast<const T*>(*it);
					
					for (size_t i = 0; i < numInPage && index < dstSize; ++i)
					{
						dstPtr[index] = *(pageDataPtr + i);
						index++;
					}
				}
			}

		private:
			PagedAtomicLinearAllocator<sizeof(T) * 1024> m_allocator;
		};

		struct LineSphere
		{
			Vector<glm::vec3> circleXZ;
			Vector<glm::vec3> circleXY;
			Vector<glm::vec3> circleYZ;

		} m_lineSphere;

		struct BillboardInstance
		{
			glm::vec3 position;
			uint32_t userData;
			glm::vec3 size;
			uint32_t isViewSpacePosition;
			glm::vec4 color;
		};

		struct BillboardDrawCommand
		{
			glm::vec3 position;
			glm::vec3 size;
			glm::vec4 color;
			uint32_t userData;
			uint32_t isViewSpacePosition;

			RefPtr<RHI::Image> texture;
		};

		struct BillboardInstancingRange
		{
			uint32_t begin;
			uint32_t count;

			RefPtr<RHI::Image> texture;
		};

		struct MeshDrawCommand
		{
			Ref<Mesh> mesh;
			Ref<RenderMaterial> material;

			TQS transform;
			uint32_t userData;
		};

		void IniitalizeLineSphere();

		RGBufferRef PrepareBillboardInstancesForRendering(RenderGraph& renderGraph);

		void RenderDebugLines(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture);
		void RenderDebugBillboards(RenderGraph& renderGraph, const RenderView& view, RGTextureRef dstTexture, RGTextureRef depthTexture);

		void DrawLineWithVertices(LineVertex* vertex0, LineVertex* vertex1, const glm::vec3& v0, const glm::vec3& v1, const glm::vec4& color);

		Allocator<LineVertex> m_lineVerticesAllocator;
		Allocator<BillboardDrawCommand> m_billboardDrawCommandAllocator;
		Allocator<MeshDrawCommand> m_meshDrawCommandAllocator;

		Vector<BillboardInstancingRange> m_billboardInstancingRanges;
	};
}
