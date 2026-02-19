#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/RenderGraphAllocators.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/RenderGraph/RenderGraphResourceManager.h"
#include "RenderCore/RenderGraph/RenderGraphCompiledPass.h"
#include "RenderCore/RenderGraph/RenderGraphContainerAllocator.h"

#include <JobSystem/Job.h>

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/Core/ResourceStateTracker.h>

#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Allocators/LinearAllocator.h>

namespace Volt
{
	namespace RHI
	{
		class CommandBuffer;
		struct ResourceState;
	}

	class GPUReadbackBuffer;
	class GPUReadbackTexture;
	class RenderGraph;

	class RenderGraphShaderParameterUniformBuffer
	{
	public:
		inline static constexpr uint64_t PerStageUniformBufferSize = 1024;

		RenderGraphShaderParameterUniformBuffer(RenderGraph& renderGraph);

		VT_INLINE RGUniformBufferSRVRef GetSRV() { return m_srv; }
		VT_INLINE uint8_t* GetMappedPointer() const { return reinterpret_cast<uint8_t*>(m_mappedPtr); }
		
		void Map();
		void Unmap();

		uint64_t Allocate(uint64_t size);

	private:
		RGUniformBufferSRVRef m_srv;
		RGUniformBufferRef m_uniformBuffer;
		void* m_mappedPtr;
	
		std::atomic_uint64_t m_head;
	};

	class VTRC_API RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

		RenderGraph(RenderGraph&& other) noexcept;
		RenderGraph& operator=(RenderGraph&& other) noexcept;

		RenderGraph(const RenderGraph& other) = delete;
		RenderGraph& operator=(const RenderGraph& other) = delete;

		RGBufferRef CreateBuffer(const RGBufferDesc& desc);
		RGTextureRef CreateTexture(const RGTextureDesc& desc);
		RGUniformBufferRef CreateUniformBuffer(const RGUniformBufferDesc& desc);

		RGBufferSRVRef CreateSRV(const RGBufferSRVDesc& desc);
		RGBufferUAVRef CreateUAV(const RGBufferUAVDesc& desc);
		RGBufferSRVRef CreateSRV(RGBufferRef buffer);
		RGBufferUAVRef CreateUAV(RGBufferRef buffer);
		RGBufferSRVRef CreateSRV(RGBufferRef buffer, RHI::PixelFormat format);
		RGBufferUAVRef CreateUAV(RGBufferRef buffer, RHI::PixelFormat format);

		RGTextureSRVRef CreateSRV(const RGTextureSRVDesc& desc);
		RGTextureUAVRef CreateUAV(const RGTextureUAVDesc& desc);
		RGTextureSRVRef CreateSRV(RGTextureRef texture);
		RGTextureUAVRef CreateUAV(RGTextureRef texture);

		RGBufferRef RegisterExternalBuffer(RefPtr<RHI::Buffer> buffer);
		RGUniformBufferRef RegisterExternalUniformBuffer(RefPtr<RHI::UniformBuffer> uniformBuffer);
		RGTextureRef RegisterExternalTexture(RefPtr<RHI::Image> texture);

		Ref<GPUReadbackBuffer> EnqueueBufferReadback(RGBufferRef srcBuffer);
		Ref<GPUReadbackTexture> EnqueueTextureReadback(RGTextureRef srcTexture);

		void EnqueueTextureExtraction(RGTextureRef texture, RefPtr<RHI::Image>* outImage);
		void EnqueueBufferExtraction(RGBufferRef buffer, RefPtr<RHI::Buffer>* outBuffer);

		void BeginMarker(const std::string& markerName, const glm::vec4& markerColor = 1.f);
		void EndMarker();

#if 0
		void AddResourceBarrier(RGResourceRef resourceHandle, const RHI::ResourceState& barrierInfo);
#endif

		template<typename T>
		VT_INLINE T* AllocParameters();
		VT_INLINE void* AllocData(size_t size);

		template<typename ParameterStruct, typename ExecFunc>
		void AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc);

		void Compile();

		// NOTE: After calling Execute the RenderGraph object is no longer valid to use!
		void Execute();
		JobCounterRef ExecuteAndExtractCounter();
		void ExecuteImmediate();
		void ExecuteImmediateAndWait();

	protected:
		friend class RenderContext;
		friend class RenderGraphShaderParameterUniformBuffer;

		struct TextureExtractionInfo
		{
			RGTextureRef texture;
			RefPtr<RHI::Image>* outImagePtr = nullptr;
		};

		struct BufferExtractionInfo
		{
			RGBufferRef buffer;
			RefPtr<RHI::Buffer>* outBufferPtr = nullptr;
		};

	protected:
		class StandaloneBarriers
		{
		public:
			struct ResourceUsageInfo
			{
				RGResourceRef resource;
				RGResourceType type;
				RHI::ResourceState newState;
			};

			VT_NODISCARD VT_INLINE ResourceUsageInfo& AddBarrier(uint32_t passIndex) { return m_passBarriers[passIndex].emplace_back(); }
			VT_NODISCARD VT_INLINE std::span<const ResourceUsageInfo> GetPassBarriers(uint32_t passIndex) const { return m_passBarriers.at(passIndex); }
			VT_NODISCARD VT_INLINE bool HasPassBarriers(uint32_t passIndex) const { return m_passBarriers.contains(passIndex) && !m_passBarriers.at(passIndex).empty(); }

		private:
			Map<uint32_t, Vector<ResourceUsageInfo>> m_passBarriers;
		};

		class StandaloneMarkers
		{
		public:
			struct MarkerInfo
			{
				std::string markerName;
				glm::vec4 markerColor;
				bool isEnd;
			};

			void BeginMarker(uint32_t passIndex, const std::string& markerName, const glm::vec4& color);
			void EndMarker(uint32_t passIndex);

			VT_NODISCARD VT_INLINE bool PassHasMarkers(uint32_t passIndex) const { return m_markers.contains(passIndex); }
			VT_NODISCARD VT_INLINE const Vector<MarkerInfo>& GetMarkersForPassIndex(uint32_t passIndex) { return m_markers.at(passIndex); }

		private:
			Map<uint32_t, Vector<MarkerInfo>> m_markers;
		};

		class RGDataAllocatorContainer
		{
		public:
			RGDataAllocatorContainer();
			~RGDataAllocatorContainer();

			RGDataAllocatorContainer(RGDataAllocatorContainer&& other) noexcept;
			RGDataAllocatorContainer& operator=(RGDataAllocatorContainer&& other) noexcept;

			VT_NODISCARD VT_INLINE RenderGraphDataAllocator* Get() { return m_allocator; }

		private:
			RenderGraphDataAllocator* m_allocator;
		};

		using ExternalResourceRegistry = Map<RawPtr<RHI::RHIResource>, RGResourceRef>;

		void SetupAllocators();

		JobCounterRef ExecuteInternal(bool isImmediate, bool waitForSync, bool extractCounter);
		void ExtractResources();
		void TransitionExternalResources();
		void PrepareResourcesForExecution();
		void CreateResourceViews();

		void SetupPass(RGPassRef pass);
		void SetupPassParameters(RGPassRef pass);
		void SetupPassDependencies(RGPassRef pass);

		void CullPasses();
		void FindResourceLifetimes();
		void BuildPassBarriers();
		void AssignExternalResourcesSrcState();

		void TransitionExternalResource(RGBufferRef buffer);
		void TransitionExternalResource(RGTextureRef texture);
		void TransitionExternalResource(RGUniformBufferRef buffer);

		void InsertBarriersIntoCommandBuffer(const RGCompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer);
		void InsertStandaloneMarkersIntoCommandBuffer(const uint32_t passIndex, const RefPtr<RHI::CommandBuffer>& commandBuffer);

		RGResourceRef TryGetRegisteredExternalResource(RawPtr<RHI::RHIResource> resource);
		void RegisterExternalResource(RawPtr<RHI::RHIResource> resource, RGResourceRef handle);

		RefPtr<RHI::RHIResource> GetRHIResource(RGResourceRef resource);

		RGSubResourceState* AllocateSubResourceState();
		void AddPassDependency(RGPassRef pass, RGResourceType resourceType, uint32_t subResourceIndex, RGSubResourceState& subResourceState, const RGResourceAccessState& lastAccess);

		// Validation
		void ValidateTextureUAV(const RGTextureUAVDesc& uavDesc);
		void ValidateAddPass(RGPassRef pass);

		// Private because we don't need to create a uniform buffer SRV
		// outside of the Render Graph.
		RGUniformBufferSRVRef CreateSRV(const RGUniformBufferSRVDesc& desc);

		// #NOTE: Must lay first to ensure correct construction/destruction order.
		RGDataAllocatorContainer m_dataAllocator;

		RenderGraphResourceManager m_resourceManager;
		ExternalResourceRegistry m_registeredExternalResources;
		StandaloneBarriers m_standaloneBarriers;
		StandaloneMarkers m_standaloneMarkers;

		RenderGraphResourceAllocator m_resourceAllocator; // Allocator for actual resources (Buffers, Textures)
		RenderGraphResourceAllocator m_resourceAccessorAllocator; // Allocator for resource accessors (SRVs, UAVs)
		RenderGraphResourceAllocator m_passParametersAllocator; // Allocator for pass parameters
		RenderGraphPassAllocator m_passAllocator; // Allocator for RenderGraph passes.
	
		RGVector<TextureExtractionInfo> m_textureExtractions;
		RGVector<BufferExtractionInfo> m_bufferExtractions;

		RGVector<RGPassRef> m_renderPasses;
		RGVector<RGResourceRef> m_resources;
		RGVector<RGResourceSRVRef> m_resourceSRVs;
		RGVector<RGResourceUAVRef> m_resourceUAVs;

		// #TODO_Ivar: A hacky way to create views for all render targets, since they aren't UAVs
		// or SRVs. Needs to be reworked.
		RGVector<ShaderParameterRenderTargetDecl> m_renderTargets;
		RGVector<RGCompiledPass> m_compiledRenderPasses;

		RefPtr<RHI::Fence> m_executionFence;
	};
}

#include "RenderCore/RenderGraph/RenderGraph.inl"
