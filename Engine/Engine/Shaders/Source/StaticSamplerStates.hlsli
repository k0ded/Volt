#pragma once

SamplerState StaticPointSampler : register(s0, space10);
SamplerState StaticBilinearSampler : register(s1, space10);
SamplerState StaticTrilinearSampler : register(s2, space10);
SamplerState StaticAnisotropicSampler : register(s3, space10);