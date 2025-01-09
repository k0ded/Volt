#pragma once

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt
{
	class RenderGraph;
	class RenderGraphBlackboard;
	class RenderScene;

	namespace RHI
	{
		class Image;
	}

	class DDGI
	{
	public:
		void Render(RenderGraph& renderGraph, RenderGraphBlackboard& blackboard, Ref<RenderScene> renderScene);

	private:
		RefPtr<RHI::Image> m_probeIrradianceAtlas;
	};
}
