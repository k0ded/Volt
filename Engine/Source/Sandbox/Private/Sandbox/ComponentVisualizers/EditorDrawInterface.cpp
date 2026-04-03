#include "sbpch.h"

#include "Sandbox/Utility/EditorResources.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterface.h"

#include <Volt-Renderer/Debug/DebugRenderer.h>

#include <unordered_set>

void EditorDrawInterface::DrawIcon(IntRef<Volt::RHI::Image> texture, const TQS& transform, DebugRenderingLayer layer, bool excludeFromGrid)
{
	DrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.texture = texture;
	drawCommand.visProxyId = -1;
	drawCommand.transform = transform;
	drawCommand.excludeFromGrid = excludeFromGrid;
	drawCommand.color = 1.f;
	drawCommand.layer = layer;
}

void EditorDrawInterface::DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material, const TQS& transform, DebugRenderingLayer layer)
{
	DrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.mesh = mesh;
	drawCommand.material = material;
	drawCommand.visProxyId = -1;
	drawCommand.transform = transform;
	drawCommand.excludeFromGrid = false;
	drawCommand.color = 1.f;
	drawCommand.layer = layer;
}

void EditorDrawInterface::DrawMesh(Ref<Volt::Mesh> mesh, const glm::vec4& color, const TQS& transform, DebugRenderingLayer layer)
{
	DrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.mesh = mesh;
	drawCommand.visProxyId = -1;
	drawCommand.transform = transform;
	drawCommand.color = color;
	drawCommand.excludeFromGrid = false;
	drawCommand.layer = layer;

	if (color.a < 1.f)
	{
		drawCommand.material = Volt::RendererUtilities::GetDefaultResources().defaultTranslucentMaterial;
	}
	else
	{
		drawCommand.material = Volt::RendererUtilities::GetDefaultResources().defaultMaterial;
	}
}

void EditorDrawInterface::Render(Volt::DebugRenderer& debugRenderer, const glm::mat4& viewMatrix, Volt::EntityID entityId, const TQS& entityTransform, float scale, float alpha)
{
	// If no draw commands are recorded, we will draw a default entity icon.
	if (m_drawCommands.empty())
	{
		const glm::vec4 userData = { std::bit_cast<float>(entityId), 0.f, 0.f, 0.f };
		IntRef<Volt::RHI::Image> gizmoTexture = EditorResources::GetEditorIcon(EditorIcon::EntityGizmo);
		debugRenderer.DrawBillboard(entityTransform.translation, scale, { 1.f, 1.f, 1.f, alpha }, gizmoTexture, userData);
	
		return;
	}

	uint32_t numIconsInGrid = 0;

	for (const DrawCommand& drawCommand : m_drawCommands)
	{
		EditorDrawInterfaceUserData userData =
		{
			entityId,
			drawCommand.visProxyId,
			drawCommand.color,
			drawCommand.layer
		};

		const glm::vec4 packedUserData = EditorDrawInterfaceUserData::Pack(userData);

		if (drawCommand.mesh)
		{
			debugRenderer.DrawMesh(drawCommand.mesh, drawCommand.material, drawCommand.transform, packedUserData);
		}
		else if (drawCommand.texture)
		{
			if (drawCommand.excludeFromGrid)
			{
				debugRenderer.DrawBillboard(drawCommand.transform.translation, drawCommand.transform.scale, glm::vec4{ 1.f, 1.f, 1.f, 1.f }, packedUserData);
			}
			else
			{
				numIconsInGrid++;
			}
		}
	}

	// Draw icons in grid
	if (numIconsInGrid > 0)
	{
		constexpr float Padding = 0.f;
		constexpr float BaseSize = 100.f;

		constexpr uint32_t NumIconsPerRow = 3;
		const uint32_t numRows = Math::DivideRoundUp(numIconsInGrid, NumIconsPerRow);

		const glm::vec3 viewSpacePosition = viewMatrix * glm::vec4(entityTransform.translation, 1.f);

		uint32_t numIconsLeft = numIconsInGrid;

		const float iconSize = scale * BaseSize;
		const float totalHeight = numRows * iconSize + Padding * (numRows - 1);

		auto drawCommandIt = m_drawCommands.begin();

		for (size_t i = 0; i < numRows; ++i)
		{
			const uint32_t numIconsInRow = std::min(numIconsLeft, NumIconsPerRow);

			// Find the y offset of this row.
			const float totalRowWidth = numIconsInRow * iconSize + Padding * (numIconsInRow - 1);
			const float yOffset = (totalHeight / numRows * i) - totalHeight * 0.5f + Padding * i + iconSize * 0.5f;

			for (size_t j = 0; j < numIconsInRow; ++j)
			{
				for (; drawCommandIt != m_drawCommands.end(); ++drawCommandIt)
				{
					if (drawCommandIt->texture && !drawCommandIt->excludeFromGrid)
					{
						// Find the x offset of this icon.
						const float xOffset = (totalRowWidth / numIconsInRow * j) - totalRowWidth * 0.5f + Padding * j + iconSize * 0.5f;

						EditorDrawInterfaceUserData userData =
						{
							entityId,
							drawCommandIt->visProxyId,
							drawCommandIt->color,
							drawCommandIt->layer
						};

						const glm::vec4 packedUserData = EditorDrawInterfaceUserData::Pack(userData);

						debugRenderer.DrawBillboard(viewSpacePosition - glm::vec3(xOffset, yOffset, 0.f), scale, glm::vec4{ 1.f, 1.f, 1.f, alpha }, drawCommandIt->texture, packedUserData, true);

						break;
					}
				}
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
			return Math::HashCombine(hash<Ref<Volt::Mesh>>()(value.mesh), hash<Ref<Volt::RenderMaterial>>()(value.material));
		}
	};
}

int32_t EditorDrawInterface::GetNextVisProxyId()
{
	return m_currentVisProxyId++;
}
