#pragma once

float CalculateExposure(float ev100)
{
    return 1.f / (pow(2.f, ev100) * 1.2f);
}