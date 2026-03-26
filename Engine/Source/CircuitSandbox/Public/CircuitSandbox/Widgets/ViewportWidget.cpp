#include "csbpch.h"
#include "ViewportWidget.h"

#include <Circuit/CircuitPainter.h>

#include <Volt-Renderer/SceneRenderer.h>

#include <Volt-Scene/Scene.h>

ViewportWidget::ViewportWidget()
{}

ViewportWidget::~ViewportWidget()
{}

void ViewportWidget::Build(const Arguments& args)
{
	m_sceneRenderer = args._SceneRenderer;
	m_scene = args._Scene;
}

glm::vec2 ViewportWidget::GetDesiredSize()
{
	return { -1,-1 };
}

void ViewportWidget::OnPaint(Circuit::CircuitPainter& painter)
{
	if (m_sceneRenderer)
	{
		if (m_prevAllottedPaintSize != painter.GetAllottedSize())
		{
			m_prevAllottedPaintSize = painter.GetAllottedSize();
			m_sceneRenderer->Resize(static_cast<uint32_t>(m_prevAllottedPaintSize.x), static_cast<uint32_t>(m_prevAllottedPaintSize.y));
			m_scene->SetRenderSize(static_cast<uint32_t>(m_prevAllottedPaintSize.x), static_cast<uint32_t>(m_prevAllottedPaintSize.y));
		}
		painter.AddImage(0, 0, painter.GetAllottedSize().x, painter.GetAllottedSize().y, m_sceneRenderer->GetFinalImage());
	}
	else
	{
		painter.AddCircle(0, 0, 50, 0xff0000ff);
	}
}
