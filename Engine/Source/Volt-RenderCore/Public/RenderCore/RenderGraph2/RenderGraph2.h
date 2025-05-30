#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph2/Resources/ResourceDeclarations.h"
#include "RenderCore/RenderGraph2/RenderGraphAllocators2.h"
#include "RenderCore/RenderGraph2/ShaderParameterStruct2.h"
#include "RenderCore/RenderGraph/SharedRenderContext.h"
#include "RenderCore/TransientResourceSystem/TransientResourceSystem2.h"

#include <RHIModule/Buffers/CommandBuffer.h>

#include <CoreUtilities/Pointers/RefPtr.h>
#include <CoreUtilities/EnumUtils.h>

namespace Volt
{
	namespace RHI
	{
		class CommandBuffer;
	}

	class VTRC_API RenderGraph2
	{
	public:
		RenderGraph2(RefPtr<RHI::CommandBuffer> commandBuffer);
		~RenderGraph2();

		RGBufferRef CreateBuffer(const RGBufferDesc& desc);
		RGTextureRef CreateTexture(const RGTextureDesc& desc);
		RGUniformBufferRef CreateUniformBuffer(const RGUniformBufferDesc& desc);

		RGBufferSRVRef CreateSRV(const RGBufferSRVDesc& desc);
		RGBufferUAVRef CreateUAV(const RGBufferUAVDesc& desc);
		RGBufferSRVRef CreateSRV(RGBufferRef buffer);
		RGBufferUAVRef CreateUAV(RGBufferRef buffer);

		RGTextureSRVRef CreateSRV(const RGTextureSRVDesc& desc);
		RGTextureUAVRef CreateUAV(const RGTextureUAVDesc& desc);
		RGTextureSRVRef CreateSRV(RGTextureRef texture);
		RGTextureUAVRef CreateUAV(RGTextureRef texture);

		RGBufferRef RegisterExternalBuffer(RefPtr<RHI::StorageBuffer> buffer);
		RGUniformBufferRef RegisterExternalUniformBuffer(RefPtr<RHI::UniformBuffer> uniformBuffer);
		RGTextureRef RegisterExternalTexture(RefPtr<RHI::Image> texture);

		template<typename T>
		T* AllocParameters()
		{
			return m_passParametersAllocator.Allocate<T>();
		}

		template<typename ParameterStruct, typename ExecFunc>
		void AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc);

		void Compile();
		void Execute();

	private:
		friend class RenderContext2;

		class CompiledPass
		{
		public:
			struct BarrierInfo
			{
				RHI::ResourceBarrierInfo barrier;
				RGResourceRef resource = nullptr;
			};

			class PassBarriers
			{
			public:
				VT_NODISCARD VT_INLINE std::span<const BarrierInfo> GetBarriers() const { return m_barriers; }
				VT_NODISCARD VT_INLINE size_t GetBarrierCount() const { return m_barriers.size(); }
				VT_NODISCARD VT_INLINE bool Empty() const { return m_barriers.empty(); }

				VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& AddBarrier(RHI::BarrierType type, RGResourceRef resource = nullptr)
				{
					auto& barrierInfo = m_barriers.emplace_back();
					barrierInfo.barrier.type = type;
					barrierInfo.resource = resource;
					return barrierInfo.barrier;
				}

				VT_NODISCARD VT_INLINE RHI::ResourceBarrierInfo& GetBarrier(size_t index)
				{
					return m_barriers.at(index).barrier;
				}

			private:
				PagedVector<BarrierInfo> m_barriers;
			};

			VT_INLINE void SetName(const std::string& name) { m_name = name; }
			VT_INLINE void AddSurrenderableResource(RGResourceRef resource) { m_surrenderableResources.emplace_back(resource); }
			VT_NODISCARD VT_INLINE const PagedVector<RGResourceRef>& GetSurrenderableResources() const { return m_surrenderableResources; }

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

			PassBarriers prePassBarriers;
			PassBarriers postPassBarriers;

		private:
			int32_t m_globalBarrierIndex = -1;

			PagedVector<RGResourceRef> m_surrenderableResources;
			std::string_view m_name;
		};

		using ExternalResourceRegistry = vt::map<RawPtr<RHI::RHIResource>, RGResourceRef>;

		void ExecuteInternal();
		void AllocateShaderParametersBuffer();

		void InsertBarriersIntoCommandBuffer(const CompiledPass::PassBarriers& passBarriers, const RefPtr<RHI::CommandBuffer>& commandBuffer);

		RGResourceRef TryGetRegisteredExternalResource(RawPtr<RHI::RHIResource> resource);
		void RegisterExternalResource(RawPtr<RHI::RHIResource> resource, RGResourceRef handle);

		RefPtr<RHI::BufferView> GetRHIBufferSRV(RGBufferSRVRef bufferSRV);
		RefPtr<RHI::BufferView> GetRHIBufferUAV(RGBufferUAVRef bufferUAV);

		RefPtr<RHI::ImageView> GetRHITextureSRV(RGTextureSRVRef textureSRV);
		RefPtr<RHI::ImageView> GetRHITextureUAV(RGTextureUAVRef textureUAV);
		RefPtr<RHI::ImageView> GetRHITextureRT(RGTextureRef texture);

		RefPtr<RHI::RHIResource> GetRHIResource(RGResourceRef resource);

		TransientResourceSystem2 m_transientResourceSystem;
		ExternalResourceRegistry m_registeredExternalResources;

		RenderGraphResourceAllocator2 m_resourceAllocator; // Allocator for actual resources (Buffers, Textures)
		RenderGraphResourceAllocator2 m_resourceAccessorAllocator; // Allocator for resource accessors (SRVs, UAVs)
		RenderGraphResourceAllocator2 m_passParametersAllocator; // Allocator for pass parameters
		RenderGraphPassAllocator2 m_passAllocator; // Allocator for RenderGraph passes.
	
		PagedVector<Handle<RenderGraphPass>> m_passes;
		PagedVector<RGResourceRef> m_resources;

		PagedVector<CompiledPass> m_compiledPasses;

		RefPtr<RHI::CommandBuffer> m_commandBuffer;
		RefPtr<RHI::Fence> m_executionFence;
		RawPtr<RHI::UniformBuffer> m_shaderParametersUniformBuffer;

		SharedRenderContext m_sharedRenderContext;
	};

	template<typename ParameterStruct, typename ExecFunc>
	void RenderGraph2::AddPass(const std::string& name, RenderGraphPassFlags flags, const ParameterStruct* parameters, ExecFunc&& executeFunc)
	{
		Handle<RenderGraphPass> newPass = m_passAllocator.AllocatePass(name, std::forward<ExecFunc>(executeFunc));
		newPass->flags = flags;

		// Get all parameters accessed by shader.
		// #TODO_Ivar: Add support for paged vector
		// #TODO_Ivar: Consider caching these.
		Vector<ShaderParameterMetadata2> parameterStructMetadata;
		ParameterStruct::zzInternal_ProcessMembers(parameterStructMetadata);

		// We need to use const_cast here because the resource parameters need to be non-const pointers.
		uint8_t* parametersStructBytePtr = reinterpret_cast<uint8_t*>(const_cast<ParameterStruct*>(parameters));

		for (const auto& parameter : parameterStructMetadata)
		{
			uint8_t* dataPtr = &parametersStructBytePtr[parameter.structOffset];

			switch (parameter.parameterType)
			{
				case ShaderParameterType2::BufferSRV: newPass->AddResourceRead(*reinterpret_cast<RGBufferSRVRef*>(dataPtr)); break;
				case ShaderParameterType2::BufferUAV: newPass->AddResourceWrite(*reinterpret_cast<RGBufferUAVRef*>(dataPtr)); break;
				case ShaderParameterType2::TextureSRV: newPass->AddResourceRead(*reinterpret_cast<RGTextureSRVRef*>(dataPtr)); break;
				case ShaderParameterType2::TextureUAV: newPass->AddResourceWrite(*reinterpret_cast<RGBufferUAVRef*>(dataPtr)); break;
				case ShaderParameterType2::BufferAccess: newPass->AddResourceAccess(*reinterpret_cast<RGBufferRef*>(dataPtr), parameter.resourceAccessType); break;
				case ShaderParameterType2::RenderTargets:
				{
					VT_ENSURE(!EnumValueContainsFlag(flags, RenderGraphPassFlags::Compute));

					const ShaderParameterRenderTargetBindings& rtBindings = *reinterpret_cast<ShaderParameterRenderTargetBindings*>(dataPtr);

					for (size_t i = 0; i < RHI::MAX_COLOR_ATTACHMENT_COUNT; ++i)
					{
						if (rtBindings.renderTargets[i] != nullptr)
						{
							newPass->AddResourceRenderTargetAccess(rtBindings.renderTargets[i]);
						}
					}

					if (rtBindings.depthTarget != nullptr)
					{
						newPass->AddResourceRenderTargetAccess(rtBindings.depthTarget);
					}

					break;
				}
			}
		}

		m_passes.emplace_back(newPass);
	}
}
