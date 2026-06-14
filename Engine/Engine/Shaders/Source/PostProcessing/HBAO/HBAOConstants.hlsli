struct HBAOConstants
{
    float2 AOSize;
    float2 InvAOSize;
    float2 InvFullSize;
    float R2;
    float radius;
    float InvNegR2;
    float power;
    float2 padding;
};

ConstantBuffer<HBAOConstants> constants;