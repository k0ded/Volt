#include "circuitpch.h"
#include "Widgets/TextWidget.h"

#include "CircuitPainter.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetMetadataWrappers.h>

Circuit::TextWidget::TextWidget()
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

Circuit::TextWidget::~TextWidget()
{
}

void Circuit::TextWidget::Build(const Arguments& args)
{
    m_text = args._Text;
    m_size = args._Size;
    m_color = args._Color;
}

glm::vec2 Circuit::TextWidget::GetDesiredSize()
{
    return m_fontAsset->CalcTextSize(m_text.Get(), m_size);
}

void Circuit::TextWidget::OnPaint(CircuitPainter& painter)
{
    if (m_fontAsset.IsValid())
    {
        painter.AddText(0, 0, m_text.Get(), m_fontAsset, painter.GetAllottedSize().x, m_color.Get(), m_size);
    }

	//painter.AddRectOutline(0, 0, GetDesiredSize().x, GetDesiredSize().y, 0xff0000ff, 1.f);
}
