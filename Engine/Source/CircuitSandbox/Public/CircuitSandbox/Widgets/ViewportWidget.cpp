#include "csbpch.h"
#include "ViewportWidget.h"

#include <Circuit/CircuitPainter.h>

#include <Volt-Renderer/SceneRenderer.h>

ViewportWidget::ViewportWidget()
{}

ViewportWidget::~ViewportWidget()
{}

void ViewportWidget::Build(const Arguments& args)
{
	m_sceneRenderer = args._SceneRenderer;
}

glm::vec2 ViewportWidget::GetDesiredSize()
{
	return { -1,-1 };
}

void ViewportWidget::OnPaint(Circuit::CircuitPainter& painter)
{
	if (m_sceneRenderer)
	{
		painter.AddImage(0, 0, painter.GetAllotedSize().x, painter.GetAllotedSize().y, m_sceneRenderer->GetFinalImage());
	}
	else
	{
		painter.AddCircle(0, 0, 50, 0xff0000ff);
	}
}
