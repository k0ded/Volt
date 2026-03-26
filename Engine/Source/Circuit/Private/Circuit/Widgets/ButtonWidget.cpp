#include "circuitpch.h"
#include "Widgets/ButtonWidget.h"
#include "WidgetInteractionData.h"

#include "CircuitPainter.h"

//TODO: REMOVE THIS
#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetMetadataWrappers.h>

Circuit::ButtonWidget::ButtonWidget()
{
	Volt::AssetHandle firstFoundFont;

	Volt::AssetRegistryIteratorFilter filter;
	filter.AddAssetType<Volt::FontAsset>();
	g_assetManager->IterateAssetRegistryWithFilter(filter, [&](Volt::ReadOnlyAssetMetadata meta)
	{
		firstFoundFont = meta->handle;
		return false;
	});
	g_assetManager->TryGetAsset(firstFoundFont, m_fontAsset);
}

Circuit::ButtonWidget::~ButtonWidget()
{}

void Circuit::ButtonWidget::Build(const Arguments& args)
{
	m_onPressedDelegate = args._OnPressed;
	m_onReleasedDelegate = args._OnReleased;
	m_text = args._Text;
	m_content = args._Content;

	if (m_content)
	{
		AddChildWidget(m_content);
	}

	m_hovered = false;
	m_pressed = false;
	m_minSize = args._MinSize;
}

glm::vec2 Circuit::ButtonWidget::GetDesiredSize()
{
	glm::vec2 result;

	if (m_content)
	{
		result = m_content->GetDesiredSize();
	}
	else
	{
		//if we dont have content, take the whole area given
		result = { -1,-1 };
	}

	m_size = result;
	return result;
}

void Circuit::ButtonWidget::OnPaint(CircuitPainter& painter)
{
	const CircuitColor baseColor(100, 100, 100);
	const CircuitColor hoveredColor(200, 200, 200);
	const CircuitColor pressedColor(150, 150, 150);

	const CircuitColor* buttonColor = &baseColor;
	if (m_pressed)
	{
		buttonColor = &pressedColor;
	}
	else if (m_hovered)
	{
		buttonColor = &hoveredColor;
	}
	painter.AddRect(0, 0, painter.GetAllottedSize().x, painter.GetAllottedSize().y, *buttonColor);

	if (m_content)
	{
		painter.AddWidget(m_content, 0, 0, painter.GetAllottedSize().x, painter.GetAllottedSize().y);
	}
	else if (m_fontAsset.IsValid())
	{
		painter.AddText(0, 0, m_text.Get(), m_fontAsset, painter.GetAllottedSize().x, 0xff0000ff, 50.f);
	}
}

void Circuit::ButtonWidget::OnBeginHover(const WidgetInteractionData& interactionData)
{
	m_hovered = true;
}

void Circuit::ButtonWidget::OnEndHover(const WidgetInteractionData& interactionData)
{
	m_hovered = false;
	m_pressed = false;
}

void Circuit::ButtonWidget::OnPressed(const WidgetInteractionData& interactionData)
{
	m_onPressedDelegate.ExecuteIfBound(interactionData.mouseButton);
	m_pressed = true;
}

void Circuit::ButtonWidget::OnReleased(const WidgetInteractionData& interactionData)
{
	m_onReleasedDelegate.ExecuteIfBound(interactionData.mouseButton);
	m_pressed = false;
}
