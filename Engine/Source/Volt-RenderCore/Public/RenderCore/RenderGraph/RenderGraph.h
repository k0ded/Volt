#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph/RenderGraphAllocators.h"
#include "RenderCore/RenderGraph/ShaderParameterStruct.h"
#include "RenderCore/RenderGraph/RenderGraphResourceManager.h"

#include <JobSystem/Job.h>

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Images/Image.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/Core/ResourceStateTracker.h>

#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Allocators/PagedLinearAllocator.h>

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

	public:
		class CompiledPass
		{
		public:
			struct BarrierInfo
			{
				RHI::ResourceBarrierInfo barrier;
				RGResourceRef resource = nullptr;
				bool requiresExternalSrcState = false;
			};

			class PassBarriers
			{
			public:
				VT_NODISCARD VT_INLINE std::span<const BarrierInfo> GetBarriers() const { return m_barriers; }
				VT_NODISCARD VT_INLINE Vector<BarrierInfo>& GetBarriersMutable() { return m_barriers; }
				VT_NODISCARD VT_INLINE size_t GetBarrierCount() const { return m_barriers.size(); }
				VT_NODISCARD VT_INLINE bool Empty() const { return m_barriers.empty(); }

				VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& AddBarrier(RHI::BarrierType type, RGResourceRef resource = nullptr, bool requiresExternalSrcState = false)
				{
					auto& barrierInfo = m_barriers.emplace_back();
					barrierInfo.barrier.type = type;
					barrierInfo.resource = resource;
					barrierInfo.requiresExternalSrcState = requiresExternalSrcState;
					return barrierInfo.barrier;
				}

				VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& GetBarrier(size_t index)
				{
					return m_barriers.at(index).barrier;
				}

			private:
				Vector<BarrierInfo> m_barriers;
			};

			VT_INLINE void SetName(const std::string& name) { m_name = name; }
			VT_INLINE void AddSurrenderableResource(RGResourceRef resource) { m_surrenderableResources.emplace_back(resource); }
			VT_NODISCARD VT_INLINE const Vector<RGResourceRef>& GetSurrenderableResources() const { return m_surrenderableResources; }

			// We only want maximum ONE global barrier per pass. As a single global barrier
			// can represent multiple.
			inline RHI::GlobalBarrier& GetGlobalBarrier()
			{
				if (m_globalBarrierIndex == -1)
				{
					m_globalBarrierIndex = static_cast<int32_t>(prePassBarriers.GetBarrierCount());
					auto& barrier = prePassBarriers.AddBarrier(RHI::BarrierType::Global);
					return barrier.globalBarrier();
				}

				return prePassBarriers.GetBarrier(static_cast<size_t>(m_globalBarrierIndex)).globalBarrier();
			}

			inline RHI::GlobalBarrier& GetPostPassGlobalBarrier()
			{
				if (m_postPassGlobalBarrierIndex == -1)
				{
					m_postPassGlobalBarrierIndex = static_cast<int32_t>(postPassBarriers.GetBarrierCount());
					auto& barrier = postPassBarriers.AddBarrier(RHI::BarrierType::Global);
					return barrier.globalBarrier();
				}

				return postPassBarriers.GetBarrier(static_cast<size_t>(m_postPassGlobalBarrierIndex)).globalBarrier();
			}

			PassBarriers prePassBarriers;
			PassBarriers postPassBarriers;

		private:
			int32_t m_globalBarrierIndex = -1;
			int32_t m_postPassGlobalBarrierIndex = -1;

			Vector<RGResourceRef> m_surrenderableResources;
			std::string_view m_name;
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

		struct RGResourceState
		{
			Handle<RenderGraphPass> previousUsage;
			RHI::ResourceState currentState;
			bool isWriteState = false;
		};

		struct RGResourceStateTracker
		{
			inline RGResourceState& GetState(RGResourceRef resource) { return resourceStates[resource]; }
			Map<RGResourceRef, RGResourceState> resourceStates;
		};

		using ExternalResourceRegistry = Map<RawPtr<RHI::RHIResource>, RGResourceRef>;

		JobCounterRef ExecuteInternal(bool isImmediate, bool waitForSync, bool extractCounter);
		void ExtractResources();
		void TransitionExternalResources();
		void PrepareResourcesForExecution();
		void CreateResourceViews();

		void InsertBarriersIntoCommandBuffer(const CompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer);
		void InsertStandaloneMarkersIntoCommandBuffer(const uint32_t passIndex, const RefPtr<RHI::CommandBuffer>& commandBuffer);

		RGResourceRef TryGetRegisteredExternalResource(RawPtr<RHI::RHIResource> resource);
		void RegisterExternalResource(RawPtr<RHI::RHIResource> resource, RGResourceRef handle);

		RefPtr<RHI::RHIResource> GetRHIResource(RGResourceRef resource);

		// Private because we don't need to create a uniform buffer SRV
		// outside of the Render Graph.
		RGUniformBufferSRVRef CreateSRV(const RGUniformBufferSRVDesc& desc);

		RenderGraphResourceManager m_resourceManager;
		ExternalResourceRegistry m_registeredExternalResources;
		StandaloneBarriers m_standaloneBarriers;
		StandaloneMarkers m_standaloneMarkers;
		RGResourceStateTracker m_resourceStateTracker;

		RenderGraphResourceAllocator m_resourceAllocator; // Allocator for actual resources (Buffers, Textures)
		RenderGraphResourceAllocator m_resourceAccessorAllocator; // Allocator for resource accessors (SRVs, UAVs)
		RenderGraphResourceAllocator m_passParametersAllocator; // Allocator for pass parameters
		RenderGraphPassAllocator m_passAllocator; // Allocator for RenderGraph passes.
		PagedLinearAllocator<65536> m_temporaryDataAllocator; // Allocator for temporary data that needs to live during the execution of the render graph.
	
		Vector<TextureExtractionInfo> m_textureExtractions;
		Vector<BufferExtractionInfo> m_bufferExtractions;

		Vector<Handle<RenderGraphPass>> m_passes;
		Vector<RGResourceRef> m_resources;
		Vector<RGResourceSRVRef> m_resourceSRVs;
		Vector<RGResourceUAVRef> m_resourceUAVs;

		Vector<CompiledPass> m_compiledPasses;

		RefPtr<RHI::Fence> m_executionFence;

		std::mutex m_tempMutex;
	}; 

	template<typename ParameterStruct, typename ExecFunc>
	void RenderGraph::AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc)
	{
		VT_PROFILE_FUNCTION();

		Handle<RenderGraphPass> newPass = m_passAllocator.AllocatePass(name, std::forward<ExecFunc>(executeFunc));
		newPass->flags = flags;

		// Get all parameters accessed by shader.
		// #TODO_Ivar: Add support for paged vector, or inline allocator
		const Vector<ShaderParameterMetadata>& parameterStructMetadata = ParameterStruct::GetShaderParameterMetadata();

		// We need to use const_cast here because the resource parameters need to be non-const pointers.
		uint8_t* parametersStructBytePtr = reinterpret_cast<uint8_t*>(const_cast<ParameterStruct*>(parameters));

		for (const auto& parameter : parameterStructMetadata)
		{
			uint8_t* dataPtr = &parametersStructBytePtr[parameter.structOffset];

			switch (parameter.parameterType)
			{
				case ShaderParameterType::BufferSRV: newPass->AddResourceRead(*reinterpret_cast<RGBufferSRVRef*>(dataPtr)); break;
				case ShaderParameterType::BufferUAV: newPass->AddResourceWrite(*reinterpret_cast<RGBufferUAVRef*>(dataPtr)); break;
				case ShaderParameterType::TextureSRV: newPass->AddResourceRead(*reinterpret_cast<RGTextureSRVRef*>(dataPtr)); break;
				case ShaderParameterType::TextureUAV: newPass->AddResourceWrite(*reinterpret_cast<RGBufferUAVRef*>(dataPtr)); break;
				case ShaderParameterType::UniformBuffer:  
				{
					RGUniformBufferRef uniformBuffer = *reinterpret_cast<RGUniformBufferRef*>(dataPtr);
					if (uniformBuffer != nullptr)
					{
						RGUniformBufferSRVDesc srvDesc{};
						srvDesc.bufferResource = uniformBuffer;

						newPass->AddResourceRead(CreateSRV(srvDesc));
					}
					break;
				}
				case ShaderParameterType::BufferAccess: newPass->AddResourceAccess(*reinterpret_cast<RGBufferRef*>(dataPtr), parameter.resourceAccessType); break;
				case ShaderParameterType::TextureAccess: newPass->AddResourceAccess(*reinterpret_cast<RGTextureRef*>(dataPtr), parameter.resourceAccessType); break;
				case ShaderParameterType::UniformBufferAccess: newPass->AddResourceAccess(*reinterpret_cast<RGUniformBufferRef*>(dataPtr), parameter.resourceAccessType); break;
				case ShaderParameterType::RenderTargets:
				{
					VT_ENSURE(!EnumValueContainsFlag(flags, RenderGraphPassFlags::Compute));

					const ShaderParameterRenderTargetBindings& rtBindings = *reinterpret_cast<ShaderParameterRenderTargetBindings*>(dataPtr);

					for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
					{
						if (rtBindings.renderTargets[i] != nullptr)
						{
							VT_ENSURE_MSG(rtBindings.renderTargets[i]->GetDesc().usage == RHI::ImageUsage::Attachment 
								|| rtBindings.renderTargets[i]->GetDesc().usage == RHI::ImageUsage::AttachmentStorage, 
								"Render Targets must have a Attachment usage type!");
							
							newPass->AddResourceRenderTargetAccess(rtBindings.renderTargets[i]);
						}
					}

					if (rtBindings.depthTarget != nullptr)
					{
						VT_ENSURE_MSG(rtBindings.depthTarget->GetDesc().usage == RHI::ImageUsage::Attachment 
							|| rtBindings.depthTarget->GetDesc().usage == RHI::ImageUsage::AttachmentStorage, 
							"Render Targets must have a Attachment usage type!");

						newPass->AddResourceRenderTargetAccess(rtBindings.depthTarget);
					}

					break;
				}
			}
		}

		m_passes.emplace_back(newPass);
	}
}
