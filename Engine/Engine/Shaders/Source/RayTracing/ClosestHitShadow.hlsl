struct Attributes
{
    float2 bary;
};

struct ShadowPayload
{
    float visibility;
};

[shader("closesthit")]
void main(inout ShadowPayload p, in Attributes attribs)
{
    p.visibility = 0.f;
}
