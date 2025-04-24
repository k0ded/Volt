#include "Resources.hlsli"
#include "GPUScene.hlsli"

vt::RWTypedBuffer<uint> OutputBuffer;
vt::TypedBuffer<GPUMesh> InputBuffer;

[numthreads(1, 1, 1)]
void main(uint threadId : SV_DispatchThreadID)
{ 
    const GPUMesh mesh = InputBuffer.Load(0);
    const float3 pos = mesh.vertexPositionsBuffer.Load(0);

    OutputBuffer.Store(0, (uint)pos.x);
} 