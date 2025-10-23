#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/MaterialTable.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/RayTracing/RayTracingInstance.h"

#include <EntitySystem/EntityID.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY(LogScenePrimitiveData, LogVerbosity::Trace);

namespace Volt
{
	class Mesh;
	class RenderScene;
	class TempAnimator;

	struct ScenePrimitiveDescription
	{
		Ref<Mesh> primitiveMesh;
		Vector<Ref<RenderMaterial>> materials;
	};

	class VTR_API ScenePrimitiveData
	{
	public:
		ScenePrimitiveData(const EntityID& relatedEntity, RenderScene* renderScene);
		ScenePrimitiveData(const EntityID& relatedEntity, RenderScene* renderScene, Ref<TempAnimator> animator);
		~ScenePrimitiveData();

		void InitializeFromDescription(const ScenePrimitiveDescription& description);
		void Invalidate();

	private:
		void CreateScenePrimitives();
		void DestroyScenePrimitives();

		Ref<Mesh> m_primitiveMesh;
		Ref<TempAnimator> m_animator;
		MaterialTable m_primitiveMaterialTable;

		EntityID m_relatedEntity;
		RenderScene* m_renderScene;

		Vector<RenderPrimitiveID> m_renderObjects;
		RayTracingInstanceID m_rayTracingInstance = 0;

		UUID64 m_meshChangedCallbackID = 0;
	};
}
