#pragma once
//------------------------------------------------------------------------------
/**
    Flatbuffer to Nebula type conversion functions

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "flat/foundation/math.h"

namespace Flat
{
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat4;
}

namespace flatbuffers
{
///
Flat::Vec4 Pack(const Math::vec4& v);
///
Math::vec4 UnPack(const Flat::Vec4& v);
///
Flat::Vec3 Pack(const Math::vec3& v);
///
Math::vec3 UnPack(const Flat::Vec3& v);
///
Flat::Vec2 Pack(const Math::vec2& v);
///
Math::vec2 UnPack(const Flat::Vec2& v);
///
Flat::Mat4 Pack(const Math::mat4& v);
///
Math::mat4 UnPack(const Flat::Mat4& v);
}