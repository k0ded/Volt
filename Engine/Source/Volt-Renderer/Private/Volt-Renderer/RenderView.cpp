#include "vrpch.h"
#include "Volt-Renderer/RenderView.h"
#include "Volt-Renderer/Camera/Camera.h"

namespace Volt
{
	CullingInfo RenderView::GetCullingInfo() const
	{
		CullingInfo result;
		result.viewMatrix = camera->GetView();
		result.cullingFrustum = camera->GetFrustumCullingInfo();
		result.nearPlane = camera->GetNearPlane();
		result.farPlane = camera->GetFarPlane();
		result.type = CullingInfo::Type::Perspective;

		return result;
	}
}
