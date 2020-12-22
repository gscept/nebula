#pragma once

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "flat/foundation/math.h"

namespace flat {
struct Vec2;
struct Vec3;
struct Vec4;
struct Mat4;
}

namespace flatbuffers
{
flat::Vec4 Pack(const Math::vec4& v);
Math::vec4 UnPack(const flat::Vec4& v);

flat::Vec3 Pack(const Math::vec3& v);
Math::vec3 UnPack(const flat::Vec3& v);
flat::Vec2 Pack(const Math::vec2& v);
Math::vec2 UnPack(const flat::Vec2& v);
flat::Mat4 Pack(const Math::mat4& v);
Math::mat4 UnPack(const flat::Mat4& v);


}