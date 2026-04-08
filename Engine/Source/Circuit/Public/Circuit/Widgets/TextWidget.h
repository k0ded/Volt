#pragma once
#include "Circuit/Widgets/Widget.h"
#include "Circuit/CircuitColor.h"

#include <Volt-Assets/FontAsset.h>
#include <AssetSystem/AssetReference.h>

#include <CoreUtilities/String/VoltString.h>

namespace Circuit
{
    class CIRCUIT_API TextWidget : public Widget
    {
    public:
        TextWidget();
        ~TextWidget() override;

        CIRCUIT_BEGIN_ARGS(TextWidget): 
			_Color(0xffffffff),
            _Size(21.f)
        {
        };
        CIRCUIT_ATTRIBUTE(String, Text);
        CIRCUIT_ATTRIBUTE(CircuitColor, Color);
        CIRCUIT_ARGUMENT(float, Size);

        CIRCUIT_END_ARGS();

        void Build(const Arguments& args);

        glm::vec2 GetDesiredSize() override;
        void OnPaint(CircuitPainter& painter) override;

        bool IsHittestInvisible() const override { return true; };
    private:
        Volt::Attribute<String> m_text;
        Volt::Attribute<CircuitColor> m_color;
        float m_size;

        AssetReference<Volt::FontAsset> m_fontAsset;
    };
}
