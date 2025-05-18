#include "PushConstant.hlsli"
#include "../RenderPipeline/MeshShaderCommon.hlsli"
#include "../RenderPipeline/MeshShaderCullCommon.hlsli"

struct VertexOutput
{
    float4 position : SV_Position;
};

[numthreads(NUM_MS_THREADS, 1, 1)]
[outputtopology("triangle")]
void MainMS(uint groupThreadId : SV_GroupThreadID, uint groupId : SV_GroupID,
            in payload MeshAmplificationPayload payload,
            out indices uint3 tris[NUM_MAX_OUT_TRIS],
            out vertices VertexOutput vertices[NUM_MAX_OUT_VERTS],
            out primitives DefaultPrimitiveOutput primitives[NUM_MAX_OUT_TRIS])
{
    const ViewData viewData = View.Load();

    const PrimitiveDrawData drawData = GPUSceneData.primitiveDrawDataBuffer.Load(payload.drawId);    
    const GPUMesh mesh = GPUSceneData.meshesBuffer.Load(drawData.meshId);

    uint meshletIndex = payload.meshletIndices[groupId];

    const Meshlet meshlet = mesh.meshletsBuffer.Load(mesh.meshletStartOffset + meshletIndex);
    const uint vertexCount = meshlet.GetVertexCount();
    const uint triCount = meshlet.GetTriangleCount();    

    SetMeshOutputCounts(vertexCount, triCount);

    if (groupThreadId < vertexCount)
    {
        const uint vertexIndex = mesh.meshletDataBuffer[meshlet.GetVertexOffset() + groupThreadId] + mesh.vertexStartOffset;
        
        float4x4 skinningMatrix = IDENTITY_MATRIX;
        if (drawData.isAnimated)
        {
            skinningMatrix = GetSkinningMatrix(mesh, vertexIndex, drawData.boneOffset, GPUSceneData.bonesBuffer);
        }

        const float3 skinnedPosition = mul(skinningMatrix, float4(mesh.vertexPositionsBuffer.Load(vertexIndex), 1.f)).xyz;
        const float4 position = TransformClipPosition(mul(viewData.nonJitteredViewProjection, float4(drawData.transform.GetWorldPosition(skinnedPosition), 1.f)));

        SetupCullingPositions(groupThreadId, position, viewData.renderSize);

        vertices[groupThreadId].position = position;
    }

    GroupMemoryBarrierWithGroupSync();

    if (groupThreadId < triCount)
    {
        const uint primitive = mesh.meshletDataBuffer.Load(meshlet.GetIndexOffset() + groupThreadId);
        const uint3 indices = UnpackPrimitive(primitive);        
        tris[groupThreadId] = indices;
        primitives[groupThreadId].cullPrimitive = IsPrimitiveCulled(indices);
    }
}

struct ColorOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
    [[vt::d32f]];
};

ColorOutput MainPS(VertexOutput input)
{
    ColorOutput output;
    output.color = 1.f;
    return output;
}