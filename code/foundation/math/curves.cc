//------------------------------------------------------------------------------
//  curves.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "math/curves.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace Math
{
template struct BezierCubic<Math::vec3>;
template struct BezierCubic<Math::vec2>;
template struct BezierCubic<Math::vec4>;
}
