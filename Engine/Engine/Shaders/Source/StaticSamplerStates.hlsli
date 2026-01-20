#pragma once

SamplerState StaticPointSampler : register(s0, space10);
SamplerState StaticBilinearSampler : register(s1, space10);
SamplerState StaticTrilinearSampler : register(s2, space10);
SamplerState StaticAnisotropicSampler : register(s3, space10);

SamplerState StaticPointSamplerClamp : register(s4, space10);
SamplerState StaticBilinearSamplerClamp : register(s5, space10);
SamplerState StaticTrilinearSamplerClamp : register(s6, space10);
SamplerState StaticAnisotropicSamplerClamp : register(s7, space10);