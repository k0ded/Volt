#include "PathTracingCommon.hlsli"

[shader("miss")]
void main(inout ShadowPayload p)
{
    p.visibility = 1.f;
}