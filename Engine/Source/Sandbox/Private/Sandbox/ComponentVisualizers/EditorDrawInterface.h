#pragma once

#include "Sandbox/ComponentVisualizers/ComponentVisualizer.h"
#include "Sandbox/ComponentVisualizers/EditorDrawInterfaceUserData.h"

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Material/RenderMaterial.h>
#include <Volt-Renderer/Renderer.h>

#include <RHIModule/Images/Image.h>

#include <EntitySystem/Entity.h>

#include <CoreUtilities/Any.h>

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

struct TQS;

template<typename ComponentVisualizerType, typename VisProxyContextType>
concept HasHandleVisProxyInteractionFunc = requires(ComponentVisualizerType t, typename ComponentVisualizerType::ComponentType & c, Volt::Entity e, const VisProxyContextType& ctx)
{
	t.HandleVisProxyInteraction(c, e, ctx);
};

class VisProxyContextManager
{
public:
	struct VisProxyInfo
	{
		std::function<void(Ref<BaseComponentVisualizer> visualizer, Volt::Entity entity, const Any&)> handleHitProxyInteractionFunc;
		VoltGUID componentGuid;
		Any visProxyContext;
		int32_t visProxyId;
	};

	template<typename ComponentVisualizerType, typename VisProxyContextType>
	void AddVisProxy(const VisProxyContextType& visProxyContext, int32_t visProxyId);

	VT_INLINE bool HasAnyVisProxies() const { return !m_visProxyInfos.empty(); }
	VT_INLINE ArrayView<VisProxyInfo> GetVisProxyInfos() const { return m_visProxyInfos; }

private:
	Vector<VisProxyInfo> m_visProxyInfos;
};

template<typename ComponentVisualizerType, typename VisProxyContextType>
void VisProxyContextManager::AddVisProxy(const VisProxyContextType& visProxyContext, int32_t visProxyId)
{
	using ComponentType = typename ComponentVisualizerType::ComponentType;

	VisProxyInfo& newVisProxyInfo = m_visProxyInfos.emplace_back();
	newVisProxyInfo.visProxyContext.Emplace(visProxyContext);
	newVisProxyInfo.visProxyId = visProxyId;
	newVisProxyInfo.componentGuid = Volt::GetTypeGUID<ComponentType>();
	newVisProxyInfo.handleHitProxyInteractionFunc = [](Ref<BaseComponentVisualizer> visualizer, Volt::Entity entity, const Any& context) 
	{
		Ref<ComponentVisualizerType> typedVisualizer = std::reinterpret_pointer_cast<ComponentVisualizerType>(visualizer);
		typedVisualizer->HandleVisProxyInteraction(entity.GetComponent<ComponentType>(), entity, context.Cast<VisProxyContextType>());
	};
}

class EditorDrawInterface
{
public:
	void DrawIcon(RefPtr<Volt::RHI::Image> texture, const TQS& transform, DebugRenderingLayer layer = DebugRenderingLayer::World, bool excludeFromGrid = false);

	void DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material, const TQS& transform, DebugRenderingLayer layer = DebugRenderingLayer::World);
	void DrawMesh(Ref<Volt::Mesh> mesh, const glm::vec4& color, const TQS& transform, DebugRenderingLayer layer = DebugRenderingLayer::World);

	template<typename ComponentVisualizerType, typename VisProxyContextType>
	void DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material, const TQS& transform, const VisProxyContextType& visProxyContext, DebugRenderingLayer layer = DebugRenderingLayer::World);

	template<typename ComponentVisualizerType, typename VisProxyContextType>
	void DrawMesh(Ref<Volt::Mesh> mesh, const glm::vec4& color, const TQS& transform, const VisProxyContextType& visProxyContext, DebugRenderingLayer layer = DebugRenderingLayer::World);

	/*
		Will fill the gizmo render commands into a debug renderer.
	*/
	void Render(Volt::DebugRenderer& debugRenderer, const glm::mat4& viewMatrix, Volt::EntityID entityId, const TQS& entityTransform, float scale, float alpha);

	VisProxyContextManager&& ExtractVisProxyContextManager() { return std::move(m_visProxyContextManager); }
	VT_INLINE bool HasAnyVisProxies() const { return m_visProxyContextManager.HasAnyVisProxies(); }

private:
	struct DrawCommand
	{
		RefPtr<Volt::RHI::Image> texture;
		Ref<Volt::Mesh> mesh;
		Ref<Volt::RenderMaterial> material;

		TQS transform;
		int32_t visProxyId;
		glm::vec4 color;
		bool excludeFromGrid;
		DebugRenderingLayer layer;
	};

	int32_t GetNextVisProxyId();

	Vector<DrawCommand> m_drawCommands;

	VisProxyContextManager m_visProxyContextManager;

	int32_t m_currentVisProxyId = 0;
};

template<typename ComponentVisualizerType, typename VisProxyContextType>
void EditorDrawInterface::DrawMesh(Ref<Volt::Mesh> mesh, Ref<Volt::RenderMaterial> material, const TQS& transform, const VisProxyContextType& visProxyContext, DebugRenderingLayer layer)
{
	DrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.mesh = mesh;
	drawCommand.material = material;
	drawCommand.transform = transform;
	drawCommand.visProxyId = GetNextVisProxyId();
	drawCommand.excludeFromGrid = false;
	drawCommand.color = 1.f;
	drawCommand.layer = layer;

	if constexpr (HasHandleVisProxyInteractionFunc<ComponentVisualizerType, VisProxyContextType>)
	{
		m_visProxyContextManager.AddVisProxy<ComponentVisualizerType>(visProxyContext, drawCommand.visProxyId);
	}
}

template<typename ComponentVisualizerType, typename VisProxyContextType>
void EditorDrawInterface::DrawMesh(Ref<Volt::Mesh> mesh, const glm::vec4& color, const TQS& transform, const VisProxyContextType& visProxyContext, DebugRenderingLayer layer)
{
	DrawCommand& drawCommand = m_drawCommands.emplace_back();
	drawCommand.mesh = mesh;
	drawCommand.transform = transform;
	drawCommand.visProxyId = GetNextVisProxyId();
	drawCommand.excludeFromGrid = false;
	drawCommand.color = color;
	drawCommand.layer = layer;

	if (color.a < 1.f)
	{
		drawCommand.material = Volt::Renderer::GetDefaultResources().defaultTranslucentMaterial;
	}
	else
	{
		drawCommand.material = Volt::Renderer::GetDefaultResources().defaultMaterial;
	}

	if constexpr (HasHandleVisProxyInteractionFunc<ComponentVisualizerType, VisProxyContextType>)
	{
		m_visProxyContextManager.AddVisProxy<ComponentVisualizerType>(visProxyContext, drawCommand.visProxyId);
	}
}
