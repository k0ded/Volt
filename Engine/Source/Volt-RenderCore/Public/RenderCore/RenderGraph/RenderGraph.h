#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/RenderGraphAllocators.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/RenderGraph/RenderGraphResourceManager.h"
#include "RenderCore/RenderGraph/RenderGraphCompiledPass.h"

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

		RGBufferRef RegisterExternalBuffer(RefPtr<RHI::StorageBuffer> buffer);
		RGUniformBufferRef RegisterExternalUniformBuffer(RefPtr<RHI::UniformBuffer> uniformBuffer);
		RGTextureRef RegisterExternalTexture(RefPtr<RHI::Image> texture);

		Ref<GPUReadbackBuffer> EnqueueBufferReadback(RGBufferRef srcBuffer);
		Ref<GPUReadbackTexture> EnqueueTextureReadback(RGTextureRef srcTexture);

		void EnqueueTextureExtraction(RGTextureRef texture, RefPtr<RHI::Image>* outImage);
		void EnqueueBufferExtraction(RGBufferRef buffer, RefPtr<RHI::StorageBuffer>* outBuffer);

		void BeginMarker(const std::string& markerName, const glm::vec4& markerColor = 1.f);
		void EndMarker();

		void AddResourceBarrier(RGResourceRef resourceHandle, const RHI::ResourceState& barrierInfo);

		template<typename T>
		T* AllocParameters()
		{
			return m_passParametersAllocator.Allocate<T>();
		}

		void* AllocData(size_t size)
		{
			return m_temporaryDataAllocator.Allocate(size);
		}

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
			RefPtr<RHI::StorageBuffer>* outBufferPtr = nullptr;
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

		using ExternalResourceRegistry = Map<RawPtr<RHI::RHIResource>, RGResourceRef>;

		JobCounterRef ExecuteInternal(bool isImmediate, bool waitForSync, bool extractCounter);
		void ExtractResources();
		void TransitionExternalResources();
		void PrepareResourcesForExecution();
		void CreateResourceViews();

		void SetupPass(RGPassRef pass);
		void SetupPassParameters(RGPassRef pass);
		void SetupPassDependencies(RGPassRef pass);

		void CullPasses();
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

		// Private because we don't need to create a uniform buffer SRV
		// outside of the Render Graph.
		RGUniformBufferSRVRef CreateSRV(const RGUniformBufferSRVDesc& desc);

		RenderGraphResourceManager m_resourceManager;
		ExternalResourceRegistry m_registeredExternalResources;
		StandaloneBarriers m_standaloneBarriers;
		StandaloneMarkers m_standaloneMarkers;

		RenderGraphResourceAllocator m_resourceAllocator; // Allocator for actual resources (Buffers, Textures)
		RenderGraphResourceAllocator m_resourceAccessorAllocator; // Allocator for resource accessors (SRVs, UAVs)
		RenderGraphResourceAllocator m_passParametersAllocator; // Allocator for pass parameters
		RenderGraphPassAllocator m_passAllocator; // Allocator for RenderGraph passes.
		RenderGraphDataAllocator m_temporaryDataAllocator; // Allocator for temporary data that needs to live during the execution of the render graph.
	
		Vector<TextureExtractionInfo> m_textureExtractions;
		Vector<BufferExtractionInfo> m_bufferExtractions;

		Vector<RGPassRef> m_renderPasses;
		Vector<RGResourceRef> m_resources;
		Vector<RGResourceSRVRef> m_resourceSRVs;
		Vector<RGResourceUAVRef> m_resourceUAVs;

		Vector<RGCompiledPass> m_compiledRenderPasses;

		RefPtr<RHI::Fence> m_executionFence;
	}; 

	template<typename ParameterStruct, typename ExecFunc>
	void RenderGraph::AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc)
	{
		VT_PROFILE_SCOPE(name.c_str());

		const ShaderParameterMetadataDescription* shaderParameterStructMetadata = ParameterStruct::GetShaderParameterMetadata();

		RGPassRef newPass = m_passAllocator.AllocatePass(name, std::forward<ExecFunc>(executeFunc), parameters, shaderParameterStructMetadata);
		newPass->m_flags = flags;
		m_renderPasses.emplace_back(newPass);

		SetupPass(newPass);
	}
}
