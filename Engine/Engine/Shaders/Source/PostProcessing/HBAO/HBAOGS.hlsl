#include "Utility/FullscreenTriangleVertex.hlsli"

struct GSOut
{
    float4 pos  : SV_Position;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

uint arrayIndex;

#define USE_INLINE_PARAMETERS
#ifdef USE_INLINE_PARAMETERS
INLINE_PARAMETER_BLOCK({
	float2 jitter;
	float2 rotation;
	uint arrayIndex;
});
#else
uint arrayIndex;
#endif

uint GetArrayIndex()
{
#ifdef USE_INLINE_PARAMETERS
    return InlineParameters.arrayIndex;
#else
    return arrayIndex;
#endif
}

[maxvertexcount(3)]
[shader("geometry")]
void MainGS(in triangle FullscreenTriangleVertex input[3], inout TriangleStream<GSOut> OUT)
{
    GSOut OutVertex;

    OutVertex.LayerIndex = GetArrayIndex();

    [unroll]
    for (int VertexID = 0; VertexID < 3; VertexID++)
    {
        OutVertex.pos = input[VertexID].position;
        OUT.Append(OutVertex);
    }
}