#pragma once

#include <Volt-Renderer/SceneRendererExtension.h>

#include <CoreUtilities/Pointers/RefPtr.h>

namespace Volt::RHI
{
	class Image;
}

class ObjectIDSceneRendererExtension : public Volt::SceneRendererExtension
{
public:
	ObjectIDSceneRendererExtension(Ref<Volt::RenderScene> renderScene)
		: Volt::SceneRendererExtension(renderScene)
	{}

	~ObjectIDSceneRendererExtension() override = default;

	Volt::RenderGraphImageHandle OnRender(Volt::RenderGraph& renderGraph, Volt::RenderGraphBlackboard& blackboard, Ref<Volt::Camera> camera, Volt::RenderGraphImageHandle prevOutputImage) override;

	VT_INLINE VT_NODISCARD RefPtr<Volt::RHI::Image> GetIDImage() const { return m_objectIdImage; }

private:
	RefPtr<Volt::RHI::Image> m_objectIdImage;
};
