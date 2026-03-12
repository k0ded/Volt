#pragma once

#include "Volt-Renderer/MeshPassProcessor.h"

#include <RenderCore/RenderGraph/Resources/ResourceDeclarations.h>

#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Math/TQS.h>
#include <CoreUtilities/DestructorHelper.h>

namespace Volt
{
	class Mesh;
	class RenderMaterial;

	class DebugMeshRenderer
	{
	public:
		VTR_API void ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters);
		void PrepareMeshesForRendering(RenderGraph& renderGraph);
		void Reset();

		virtual void AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData) = 0;
		virtual bool ShouldIncludeDraw(const RenderMaterial& renderMaterial) const = 0;

		VT_INLINE RGBufferRef GetPrimitiveIndexBuffer() const { return m_primitiveIndexDataBuffer; }
		VT_INLINE RGBufferRef GetDebugMeshDataBuffer() const { return m_debugMeshDataBuffer; }
		VT_INLINE bool HasAnyDraw() const { return !m_meshDrawCommandBuckets.empty(); }

	protected:
		VTR_API void BuildMeshDrawCommand(Ref<Mesh> mesh, 
			Ref<RenderMaterial> renderMaterial, 
			const TQS& transform, 
			const glm::vec4& userData,
			RHI::RenderPipelineCreateInfo pipelineInfo, 
			RefPtr<RHI::Shader> vertexShader, 
			RefPtr<RHI::Shader> pixelShader);

	private:
		struct MeshDrawCommandBucket
		{
			struct InstancingRange
			{
				uint32_t offset;
				uint32_t count;
			};

			struct MeshDrawCommandInfo
			{
				MeshDrawCommand drawCommand;

				TQS transform;
				Ref<RenderMaterial> material;
				glm::vec4 userData;
			};

			Vector<MeshDrawCommandInfo> drawCommands;
			Vector<InstancingRange> instancingRanges;
		};

		MeshDrawCommandBucket& GetOrCreateBucket(MeshDrawCommandHashKey hashKey);
		MeshDrawCommandSortKey GetSortKeyFromMaterial(RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader, RenderMaterial& renderMaterial);
		uint64_t GetMaterialPermutationHash(RenderMaterial& renderMaterial);

		Vector<MeshDrawCommandBucket> m_meshDrawCommandBuckets;
		Map<MeshDrawCommandHashKey, size_t> m_hashKeyToBucketIndex;

		RGBufferRef m_primitiveIndexDataBuffer = nullptr;
		RGBufferRef m_debugMeshDataBuffer = nullptr;
	};

	class DebugMeshRendererRegistry
	{
	public:
		VTR_API ~DebugMeshRendererRegistry();

		void AddMeshDraw(Ref<Mesh> mesh, Ref<RenderMaterial> renderMaterial, const TQS& transform, const glm::vec4& userData);
		void PrepareMeshesForRendering(RenderGraph& renderGraph);
		void Reset();

		template<typename T>
			requires(std::is_base_of_v<DebugMeshRenderer, T>)
		T* AddRenderer()
		{
			constexpr size_t AllocationSize = sizeof(T);

			void* alloc = m_debugMeshRendererAllocator.Allocate(AllocationSize);
			T* processor = new (alloc) T();

			m_debugMeshRendererDestructors.emplace_back() = DestructorHelper::Create<T>(alloc);
			m_debugMeshRenderers.emplace_back(TypeTraits::TypeIndex::FromType<T>(), processor);

			return processor;
		}

		template<typename T>
			requires(std::is_base_of_v<DebugMeshRenderer, T>)
		T* GetRenderer()
		{
			constexpr TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<T>();

			for (const DebugMeshRendererContainer& container : m_debugMeshRenderers)
			{
				if (container.typeIndex == typeIndex)
				{
					return reinterpret_cast<T*>(container.debugMeshRenderer);
				}
			}

			return nullptr;
		}

	private:
		struct DebugMeshRendererContainer
		{
			TypeTraits::TypeIndex typeIndex = TypeTraits::TypeIndex::FromType<void>();
			DebugMeshRenderer* debugMeshRenderer = nullptr;
		};

		PagedAtomicLinearAllocator<1024> m_debugMeshRendererAllocator;
		Vector<DebugMeshRendererContainer> m_debugMeshRenderers;
		Vector<DestructorHelper> m_debugMeshRendererDestructors;
	};
}
