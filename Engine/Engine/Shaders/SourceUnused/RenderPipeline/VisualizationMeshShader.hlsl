#include "PushConstant.hlsli"

#include "MeshShaderCommon.hlsli"
#include "MeshShaderCullCommon.hlsli"
#include "VisualizationCommon.hlsli"

uint VisualizationModeInt; 

struct VertexOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

groupshared MeshAmplificationPayload m_payload;

[numthreads(NUM_AS_THREADS, 1, 1)]
void MainAS(uint groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    const uint taskIndex = groupId.x * NUM_AS_THREADS + groupId.y;

    const MeshTaskCommand command = TaskCommands.Load(taskIndex);
    const PrimitiveDrawData drawData = GPUSceneData.primitiveDrawDataBuffer.Load(command.drawId);    
    const GPUMesh mesh = GPUSceneData.meshesBuffer.Load(drawData.meshId);

    const uint meshletIndex = command.meshletOffset + groupThreadId;
 
    bool visible = false;

    if (groupThreadId < command.taskCount && meshletIndex < mesh.meshletCount)
    {
        const Meshlet meshlet = mesh.meshletsBuffer.Load(mesh.meshletStartOffset + meshletIndex);       
        const ViewData viewData = View.Load(); 
        
        const float3 center = drawData.transform.GetWorldPosition(meshlet.boundingSphereCenter);
        const float3 viewCenter = mul(viewData.view, float4(center, 1.f)).xyz;
        const float radius = meshlet.boundingSphereRadius * max(drawData.transform.scale.x, max(drawData.transform.scale.y, drawData.transform.scale.z));
        
        const float3 coneAxis = drawData.transform.RotateVector(meshlet.GetConeAxis());
        const float coneCutoff = meshlet.GetConeCutoff();        

        visible = true; //!ConeCull(center, radius, coneAxis, coneCutoff, viewData.cameraPosition.xyz);
        visible = visible && viewCenter.z * viewData.cullingFrustum.y - abs(viewCenter.x) * viewData.cullingFrustum.x > -radius;
        visible = visible && viewCenter.z * viewData.cullingFrustum.w - abs(viewCenter.y) * viewData.cullingFrustum.z > -radius;
    }

    if (visible)
    {
        uint index = WavePrefixCountBits(visible);
        m_payload.meshletIndices[index] = meshletIndex;
        m_payload.drawId = command.drawId;
    }

    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, m_payload);
}

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
        const float3x3 cameraNormalRotation = (float3x3)viewData.view;
        
        float4x4 skinningMatrix = IDENTITY_MATRIX;
        if (drawData.isAnimated)
        {
            skinningMatrix = GetSkinningMatrix(mesh, vertexIndex, drawData.boneOffset, GPUSceneData.bonesBuffer);
        }

        const float3 vertexPosition = mesh.vertexPositionsBuffer.Load(vertexIndex);

        const float3 skinnedPosition = mul(skinningMatrix, float4(vertexPosition, 1.f)).xyz;
        const float4 position = TransformClipPosition(mul(viewData.viewProjection, float4(drawData.transform.GetWorldPosition(skinnedPosition), 1.f)));

        SetupCullingPositions(groupThreadId, position, viewData.renderSize);
 
        const VertexMaterialData vertexMaterialData = mesh.vertexMaterialBuffer.Load(vertexIndex);

        const float3 vertexNormal = GetNormal(mesh, vertexIndex);
        const float3 vertexTangent = GetTangent(mesh, vertexIndex, vertexNormal);

        vertices[groupThreadId].position = position;
        vertices[groupThreadId].uv = vertexMaterialData.texCoords;
        vertices[groupThreadId].normal = normalize(drawData.transform.RotateVector(vertexNormal));
        vertices[groupThreadId].tangent = normalize(drawData.transform.RotateVector(vertexTangent));
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

    if (VisualizationModeInt == EVisualizationMode::ERM_UV)
    {
        output.color.rg = input.uv;
        output.color.b = 0.f;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_GeometryNormals)
    {
        output.color.rgb = input.normal * 0.5f + 0.5f;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_GeometryTangents)
    {
        output.color.rgb = input.tangent * 0.5f + 0.5f;
    }

    return output;
}