#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Buffers/StorageBuffer.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Pointers/RefPtr.h>

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
		class StorageBuffer;
	}

	class RenderGraph;
	class VTRC_API RenderGraphDebugger
	{
	public:
		void ProcessRenderGraph(RenderGraph& renderGraph);
		void WaitForFinishedExecution() const;

		VT_NODISCARD VT_INLINE const Vector<RefPtr<RHI::Image>>& GetImages() const { return m_extractedImages; }
		VT_NODISCARD VT_INLINE const Vector<RefPtr<RHI::StorageBuffer>>& GetStorageBuffers() const { return m_extractedBuffers; }

	private:
		Vector<RefPtr<RHI::Image>> m_extractedImages;
		Vector<RefPtr<RHI::StorageBuffer>> m_extractedBuffers;
	};
}
