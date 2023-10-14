//------------------------------------------------------------------------------
//  nebula_flat.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nebula_flat.h"
#include "flat/foundation/math.h"

namespace flatbuffers
{
//------------------------------------------------------------------------------
/**
*/
Flat::Mat4 Pack(const Math::mat4& v)
{
    Flat::Mat4 V;
    v.store(V.mutable_mat4()->data());
    return V;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4 UnPack(const Flat::Mat4& v)
{
    Math::mat4 m;
    m.load(v.mat4()->data());
    return m;
}

//------------------------------------------------------------------------------
/**
*/
Flat::Vec2 Pack(const Math::vec2& v)
{
    Flat::Vec2 V(v.x, v.y);
    return V;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec2 UnPack(const Flat::Vec2& v)
{
    Math::vec2 m(v.x(), v.y());
    return m;
}

//------------------------------------------------------------------------------
/**
*/
Flat::Vec3 Pack(const Math::vec3& v)
{
    Flat::Vec3 V(v.x, v.y, v.z);
    return V;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec3 UnPack(const Flat::Vec3& v)
{
    Math::vec3 m(v.x(), v.y(), v.z());
    return m;
}

//------------------------------------------------------------------------------
/**
*/
Flat::Vec4 Pack(const Math::vec4& v)
{
    Flat::Vec4 V(v.x, v.y, v.z, v.w);
    return V;
}

//------------------------------------------------------------------------------
/**
*/
Math::vec4 UnPack(const Flat::Vec4& v)
{
    Math::vec4 m(v.x(), v.y(), v.z(), v.w());
    return m;
}

//------------------------------------------------------------------------------
/**
*/
Flat::Quat
Pack(const Math::quat& v)
{
    Flat::Quat V(v.x, v.y, v.z, v.w);
    return V;
}

//------------------------------------------------------------------------------
/**
*/
Math::quat
UnPack(const Flat::Quat& v)
{
    Math::quat m(v.x(), v.y(), v.z(), v.w());
    return m;
}
}
