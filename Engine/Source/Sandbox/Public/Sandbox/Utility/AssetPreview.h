#pragma once

#include <EntitySystem/Entity.h>

#include <RHIModule/Images/Image.h>

#include <AssetSystem/Asset.h>

namespace Volt
{
	class Scene;
	class SceneRenderer;
	class Camera;

	namespace RHI
	{
		class Image;
	}
}

class AssetPreview
{
public:
	AssetPreview(const std::filesystem::path& path);

	void Render();
	const RefPtr<Volt::RHI::Image> GetPreview() const;
	inline const bool IsRendered() const { return m_isRenderered; }

private:
	Volt::AssetHandle m_assetHandle;
	Volt::Entity m_entity;

	bool m_isRenderered = false;

	Ref<Volt::Camera> m_camera;
	Ref<Volt::Scene> m_scene;
	Ref<Volt::SceneRenderer> m_sceneRenderer;
};
