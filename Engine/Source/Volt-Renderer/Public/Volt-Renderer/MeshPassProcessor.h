#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Material/RenderMaterial.h"

#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Pipelines/RenderPipeline.h>
#include <RHIModule/Core/RHICommon.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/FixedSizeLinearAllocator.h>
#include <CoreUtilities/DestructorHelper.h>
#include <CoreUtilities/UUID.h>

#include <type_traits>

namespace Volt
{
	struct RenderPrimitiveData;
	class RenderContext;
	class RenderGraph;
	class RenderScene;
	class BatchedShaderParameters;
	class RGBuffer;
	class JobCounter;

	struct MeshDrawCommandSortKey
	{
		union
		{
			uint64_t sortKey;

			struct SortKeyContents
			{
				uint64_t vertexShaderHash : 24u;
				uint64_t pixelShaderHash : 24u;
				uint64_t permutationHash : 16u;

			} sortKeyContents;
		};
	};

	struct MeshDrawCommandHashKey
	{
		union
		{
			uint64_t hashKey;

			struct HashKeyContents
			{
				uint64_t vertexBufferHash : 16u;
				uint64_t indexBufferHash : 16u;
				uint64_t subMeshHash : 32u;
			} hashKeyContents;
		};
	};

	VT_INLINE bool operator==(const MeshDrawCommandHashKey& lhs, const MeshDrawCommandHashKey& rhs)
	{
		return lhs.hashKey == rhs.hashKey;
	}

	struct MeshDrawCommand
	{
		RHI::VertexBufferVector vertexBuffers;
		RefPtr<RHI::StorageBuffer> indexBuffer;

		RefPtr<RHI::RenderPipeline> renderPipeline;

		const RenderPrimitiveData* renderPrimitive = nullptr;
		MeshDrawCommandSortKey sortKey;
		MeshDrawCommandHashKey hashKey;

		uint32_t primitiveIndex;

		// Draw command
		RHI::DrawIndexedIndirectCommand drawCommand;
	};

	class VTR_API MeshPassProcessor
	{
	public:
		MeshPassProcessor();
		virtual ~MeshPassProcessor();

		void PrepareRenderCommands(RenderGraph& renderGraph);

		void ExecuteCommands(RenderContext& renderContext, BatchedShaderParameters& batchedShaderParameters);

		virtual void AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive) = 0;
		virtual void RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive) = 0;

	protected:
		void BuildMeshDrawCommand(const RenderPrimitiveData* renderPrimitive, RHI::RenderPipelineCreateInfo pipelineInfo, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader);
		void RemoveMeshDrawCommand(const RenderPrimitiveData* renderPrimitive);

	private:
		struct MeshDrawCommandBucket
		{
			struct InstancingRange
			{
				uint32_t offset;
				uint32_t count;
			};

			Vector<MeshDrawCommand> drawCommands;
			Vector<InstancingRange> instancingRanges;
			bool isDirty;
		};

		MeshDrawCommandBucket& GetOrCreateBucket(MeshDrawCommandHashKey hashKey);
		MeshDrawCommandBucket* TryGetBucket(MeshDrawCommandHashKey hashKey);

		void MarkBucketDirty(MeshDrawCommandHashKey hashKey);

		MeshDrawCommandHashKey GetHashKeyFromRenderPrimitive(const RenderPrimitiveData* renderPrimitive);
		MeshDrawCommandSortKey GetSortKeyFromRenderPrimitive(const RenderPrimitiveData* renderPrimitive, RefPtr<RHI::Shader> vertexShader, RefPtr<RHI::Shader> pixelShader);
		uint64_t GetMaterialPermutationHash(Weak<RenderMaterial> renderMaterial);
		MaterialShader::InlineParameterBlock GetMaterialInlineParameterBlock(const RenderPrimitiveData* renderPrimitive) const;

		Vector<MeshDrawCommandBucket> m_meshDrawCommandBuckets;
		Vector<uint32_t> m_meshDrawCommandBucketPrimitiveOffsets;
		Map<MeshDrawCommandHashKey, size_t> m_hashKeyToBucketIndex;

		JobCounter* m_sortTaskCounter = nullptr;

		RGBuffer* m_primitiveIndexVertexBuffer = nullptr;
	};

	class MeshPassProcessorRegistry
	{
	public:
		MeshPassProcessorRegistry(RenderScene* renderScene);
		~MeshPassProcessorRegistry();

		template<typename T>
		requires (std::is_base_of_v<MeshPassProcessor, T>)
		T* AddProcessor()
		{
			constexpr size_t AllocationSize = sizeof(T);

			void* alloc = m_meshPassProcessorAllocator.Allocate(AllocationSize);
			T* processor = new (alloc) T();

			m_meshPassDestructors.emplace_back() = DestructorHelper::Create<T>(alloc);
			m_meshPassProcessors.emplace_back(processor);

			AddPrimitivesToMeshPassProcessor(processor);

			return processor;
		}

		void AddRenderPrimitive(const RenderPrimitiveData* renderPrimitive);
		void RemoveRenderPrimitive(const RenderPrimitiveData* renderPrimitive);

	private:
		void VTR_API AddPrimitivesToMeshPassProcessor(MeshPassProcessor* meshPassProcessor);

		FixedSizeLinearAllocator<> m_meshPassProcessorAllocator;
		Vector<MeshPassProcessor*> m_meshPassProcessors;
		Vector<DestructorHelper> m_meshPassDestructors;

		RenderScene* m_renderScene;
	};
}

namespace std
{
	template <typename T> struct hash;

	template<>
	struct hash<Volt::MeshDrawCommandHashKey>
	{
		std::size_t operator()(const Volt::MeshDrawCommandHashKey& hashKey) const
		{
			return static_cast<size_t>(hashKey.hashKey);
		}
	};
}
