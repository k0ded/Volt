#pragma once

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Material/RenderMaterial.h>

#include <RHIModule/Images/Image.h>

/*
	TODO:
		- Think about how to handle complex cases,
		  such as a single component wanting to draw
		  multiple icons or meshes.
*/

namespace Volt
{
	class DebugRenderer;
}

class EditorGizmoDrawer
{
public:
	void DrawIcon(RefPtr<Volt::RHI::Image> texture);
	void DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material);

	/*
		Will fill the gizmo render commands into a debug renderer.
	*/
	void Render(Volt::DebugRenderer& debugRenderer, const glm::mat4& viewMatrix, Volt::EntityID entityId, const glm::vec3& worlPosition, float scale, float alpha);

private:
	struct GizmoDrawCommand
	{
		RefPtr<Volt::RHI::Image> texture;

		Ref<Volt::Mesh> mesh;
		Ref<Volt::RenderMaterial> material;
	};

	Vector<GizmoDrawCommand> Consolidate() const;

	Vector<GizmoDrawCommand> m_drawCommands;
};
