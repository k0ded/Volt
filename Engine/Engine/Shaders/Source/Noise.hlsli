#pragma once

// From https://www.shadertoy.com/view/MslGR8
float RemapPDFTriUnity(float v)
{
    v = v * 2.f - 1.f;
    v = sign(v) * (1.f - sqrt(1.f - abs(v)));

    return v;
}

float3 RemapPDFTriUnity(float3 v)
{
    return float3(
        RemapPDFTriUnity(v.x),
        RemapPDFTriUnity(v.y),
        RemapPDFTriUnity(v.z)
    );
}