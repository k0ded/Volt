#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/RenderGraphPass.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/Buffer.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/ArrayView.h>
#include <CoreUtilities/Pointers/IntRef.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
		class Buffer;
	}

	class RenderGraph;
	class VTRC_API RenderGraphDebugger
	{
	public:
		struct ResourcePassAccess
		{
			uint32_t passIndex;
			bool isRead;
		};

		struct RenderGraphPass
		{
			std::string passName;
			Vector<uint32_t> resourceReads;
			Vector<uint32_t> resourceWrites;
			Vector<uint32_t> renderTargets;

			RenderGraphPassFlags passFlags = RenderGraphPassFlags::None;
			bool isCulled;
		};

		struct RenderGraphResource
		{
			std::string name;
			RGResourceType resourceType;

			uint32_t firstUsagePass;
			uint32_t lastUsagePass;

			Vector<ResourcePassAccess> passAccesses;

			bool isExternal : 1;
			bool isExtracted : 1;
			bool isProduced : 1;
		};

		using RenderGraphResourcesMap = Map<uint32_t, RenderGraphResource>;

		void Clear();
		void ProcessRenderGraph(RenderGraph& renderGraph);
		void WaitForFinishedExecution() const;

		VT_INLINE ArrayView<RenderGraphPass> GetPasses() const { return m_renderGraphPasses; }
		VT_INLINE const RenderGraphResourcesMap& GetTransientResources() const { return m_transientRenderGraphResources; }
		VT_INLINE const RenderGraphResourcesMap& GetExternalResources() const { return m_externalRenderGraphResources; }

	private:
		Vector<RenderGraphPass> m_renderGraphPasses;
		RenderGraphResourcesMap m_transientRenderGraphResources;
		RenderGraphResourcesMap m_externalRenderGraphResources;
	};
}
