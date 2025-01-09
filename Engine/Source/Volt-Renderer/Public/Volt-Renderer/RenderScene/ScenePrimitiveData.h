#pragma once

#include "Volt-Renderer/MaterialTable.h"
#include "Volt-Renderer/RenderPrimitiveData.h"
#include "Volt-Renderer/RayTracing/RayTracingInstance.h"

#include <EntitySystem/EntityHelper.h>

#include <LogModule/LogCategory.h>

VT_DECLARE_LOG_CATEGORY(LogScenePrimitiveData, LogVerbosity::Trace);

namespace Volt
{
	class Mesh;
	class RenderScene;

	struct ScenePrimitiveDescription
	{
		AssetHandle primitiveMesh;
		Vector<AssetHandle> materials;
	};

	class ScenePrimitiveData
	{
	public:
		ScenePrimitiveData(const EntityID& entityId, RenderScene* renderScene);
		~ScenePrimitiveData();

		void InitializeFromDescription(const ScenePrimitiveDescription& description);
		void Invalidate();

	private:
		void CreateScenePrimitives();
		void DestroyScenePrimitives();

		Ref<Mesh> m_primitiveMesh;
		MaterialTable m_primitiveMaterialTable;

		EntityID m_relatedEntity;
		RenderScene* m_renderScene;

		Vector<RenderPrimitiveID> m_renderObjects;
		RayTracingInstanceID m_rayTracingInstance = 0;

		UUID64 m_meshChangedCallbackID = 0;
	};
}
