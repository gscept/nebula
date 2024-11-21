#pragma once

#include "math/scalar.h"

namespace TBUI
{
#pragma pack(push, 1)
struct TBUIVertex
{
    float position[2];
    float uv[2];
    Math::byte4u color;
};
#pragma pack(pop)
}
