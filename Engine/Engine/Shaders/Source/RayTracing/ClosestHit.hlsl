struct Attributes
{
  float2 bary;
};

struct Payload
{
    float3 hitValue;
};

[shader("closesthit")]
void main(inout Payload p, in Attributes attribs)
{
  const float3 barycentricCoords = float3(1.0f - attribs.bary.x - attribs.bary.y, attribs.bary.x, attribs.bary.y);
  p.hitValue = barycentricCoords;
}
