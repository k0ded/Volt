struct Payload
{
    float3 hitValue;
};

[shader("miss")]
void main(inout Payload p)
{
    p.hitValue = float3(0.0, 0.0, 0.2);
}