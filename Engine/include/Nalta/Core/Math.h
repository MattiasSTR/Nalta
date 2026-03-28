#pragma once

// hlslpp requires these to be defined before including
#define HLSLPP_FEATURE_TRANSFORM

#include <hlsl++.h>

// Expose all hlslpp types and functions globally
using namespace hlslpp;

namespace Nalta
{
    inline float Deg2Rad(const float aDegrees) { return aDegrees * (3.14159265358979323846f / 180.0f); }
    inline float Rad2Deg(const float aRadians) { return aRadians * (180.0f / 3.14159265358979323846f); }

    constexpr float PI     { 3.14159265358979323846f };
    constexpr float TWO_PI { 6.28318530717958647692f };
    constexpr float HALF_PI{ 1.57079632679489661923f };
}