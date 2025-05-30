#pragma once

#include "RenderCore/Resources/BindlessResource.h"

#include <RHIModule/Descriptors/ResourceHandle.h>

#include <CoreUtilities/Pointers/RawPtr.h>

namespace Volt
{
	namespace RHI
	{
		class StorageBuffer;
		class UniformBuffer;
	}

	class SharedRenderContext
	{
	public:
		SharedRenderContext() = default;
		~SharedRenderContext();

		SharedRenderContext(SharedRenderContext&& other) noexcept;
		SharedRenderContext& operator=(SharedRenderContext&& other) noexcept;

		SharedRenderContext(const SharedRenderContext& other) = delete;
		SharedRenderContext& operator=(const SharedRenderContext& other) = delete;

		void BeginContext();
		void EndContext();

		uint8_t* GetRenderGraphConstantsPointer(uint32_t passIndex);
		VT_NODISCARD VT_INLINE RawPtr<RHI::UniformBuffer> GetRenderGraphConstantsBuffer() const { return m_renderGraphConstantsBuffer; }

	private:
		friend class RenderGraph;
		friend class RenderGraph2;

		void SetRenderGraphConstantsBuffer(RawPtr<RHI::UniformBuffer> constantsBuffer);

		bool m_isRenderGraphConstantsMapped = false;
		uint8_t* m_mappedRenderGraphConstantsPointer = nullptr;

		RawPtr<RHI::UniformBuffer> m_renderGraphConstantsBuffer;
	};
}
