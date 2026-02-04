#include "sbpch.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterface.h"

#include <Volt-Renderer/Debug/DebugRenderer.h>

#include <unordered_set>

void EditorDrawInterface::DrawIcon(RefPtr<Volt::RHI::Image> texture)
{
	GizmoDrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.texture = texture;
	drawCommand.visProxyId = -1;
}

void EditorDrawInterface::DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material)
{
	GizmoDrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.mesh = mesh;
	drawCommand.material = material;
	drawCommand.visProxyId = -1;
}

void EditorDrawInterface::Render(Volt::DebugRenderer& debugRenderer, const glm::mat4& viewMatrix, Volt::EntityID entityId, const TQS& transform, float scale, float alpha)
{
	const glm::vec4 userData = { std::bit_cast<float>(entityId), 0.f, 0.f, 0.f };

	// If no draw commands are recorded, we will draw a default entity icon.
	if (m_drawCommands.empty())
	{
		RefPtr<Volt::RHI::Image> gizmoTexture = EditorResources::GetEditorIcon(EditorIcon::EntityGizmo);
		debugRenderer.DrawBillboard(transform.translation, scale, { 1.f, 1.f, 1.f, alpha }, gizmoTexture, userData);
	
		return;
	}

	Vector<GizmoDrawCommand> consolidatedDrawCommands = Consolidate();

	// It's a mesh draw, there is only one.
	if (consolidatedDrawCommands.begin()->mesh != nullptr)
	{
		const GizmoDrawCommand& drawCommand = *consolidatedDrawCommands.begin();
		debugRenderer.DrawMesh(drawCommand.mesh, drawCommand.material, transform, userData);
	}
	else
	{
		constexpr float Padding = 0.f;
		constexpr float BaseSize = 100.f;

		constexpr size_t NumIconsPerRow = 3;
		const size_t numIcons = consolidatedDrawCommands.size();
		const size_t numRows = Math::DivideRoundUp(numIcons, NumIconsPerRow);
	
		const glm::vec3 viewSpacePosition = viewMatrix * glm::vec4(transform.translation, 1.f);

		size_t numIconsLeft = numIcons;
		size_t iconIndex = 0;

		const float iconSize = scale * BaseSize;
		const float totalHeight = numRows * iconSize + Padding * (numRows - 1);

		for (size_t i = 0; i < numRows; ++i)
		{
			const size_t numIconsInRow = std::min(numIconsLeft, NumIconsPerRow);

			// Find the y offset of this row.
			const float totalRowWidth = numIconsInRow * iconSize + Padding * (numIconsInRow - 1);
			const float yOffset = (totalHeight / numRows * i) - totalHeight * 0.5f + Padding * i + iconSize * 0.5f;

			for (size_t j = 0; j < numIconsInRow; ++j)
			{
				// Find the x offset of this icon.
				const float xOffset = (totalRowWidth / numIconsInRow * j) - totalRowWidth * 0.5f + Padding * j + iconSize * 0.5f;
				debugRenderer.DrawBillboard(viewSpacePosition - glm::vec3(xOffset, yOffset, 0.f), scale, glm::vec4{ 1.f, 1.f, 1.f, alpha }, consolidatedDrawCommands[iconIndex].texture, userData, true);
			
				iconIndex++;
			}

			numIconsLeft -= numIconsInRow;
		}
	}
}

struct MeshAndMaterial
{
	Ref<Volt::Mesh> mesh;
	Ref<Volt::RenderMaterial> material;

	bool operator==(const MeshAndMaterial& other) const
	{
		return mesh == other.mesh && material == other.material;
	}
};

namespace std
{
	template<>
	struct hash<MeshAndMaterial>
	{
		size_t operator()(const MeshAndMaterial& value) const
		{
			return Math::HashCombine(hash<void*>()(value.mesh.get()), hash<void*>()(value.material.get()));
		}
	};
}

Vector<EditorDrawInterface::GizmoDrawCommand> EditorDrawInterface::Consolidate() const
{
	std::unordered_set<RefPtr<Volt::RHI::Image>> individualTextures;
	std::unordered_set<MeshAndMaterial> individualMeshes;

	for (const GizmoDrawCommand& drawCommand : m_drawCommands)
	{
		// It's a billboard.
		if (drawCommand.texture)
		{
			individualTextures.insert(drawCommand.texture);
		}
		// It's a mesh
		else
		{
			individualMeshes.insert({ drawCommand.mesh, drawCommand.material });
		}
	}

	// Currently if a mesh is to be drawn, no icons will be drawn. And only one mesh will be drawn.
	if (!individualMeshes.empty())
	{
		const MeshAndMaterial& firstMesh = *individualMeshes.begin();
		return { { nullptr, firstMesh.mesh, firstMesh.material } };
	}

	Vector<GizmoDrawCommand> result;

	for (const RefPtr<Volt::RHI::Image>& texture : individualTextures)
	{
		result.emplace_back(texture, nullptr, nullptr);
	}

	return result;
}

int32_t EditorDrawInterface::GetNextVisProxyId()
{
	return m_currentVisProxyId++;
}
