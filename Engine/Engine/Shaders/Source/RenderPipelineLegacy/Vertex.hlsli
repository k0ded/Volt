#pragma once

struct Vertex
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
    uint primtiveIndex : SV_InstanceID;
};