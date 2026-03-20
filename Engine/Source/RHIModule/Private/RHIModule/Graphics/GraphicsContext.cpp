#include "rhipch.h"
#include "RHIModule/Graphics/GraphicsContext.h"

#include "RHIModule/RHIModule.h"

namespace Volt::RHI
{
	GraphicsContext::GraphicsContext()
	{
		s_context = this;
	}

	GraphicsContext::~GraphicsContext()
	{
		s_context = nullptr;
	}

	IntRef<GraphicsContext> GraphicsContext::Create(const GraphicsContextCreateInfo& createInfo)
	{
		s_graphicsAPI = createInfo.graphicsApi;
		return RHIModule::GetInstance().CreateGraphicsContext(createInfo);
	}
}
