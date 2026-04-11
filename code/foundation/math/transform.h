#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::transform
    Simple transform using position, quaternion, and scale

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/vec3.h"
#include "math/quat.h"

//------------------------------------------------------------------------------
namespace Math
{
class transform
{
public:
    /// default constructor
    transform();
    /// constructor 1
    transform(const Math::vec3& pos, const Math::quat& rot = Math::quat(), const Math::vec3& scale = Math::_plus1);
    /// set content
    void Set(const Math::vec3& pos, const Math::quat& rot, const Math::vec3& scale);
    /// get the relative transform from this to child, expressed as matrix result = this * inv(child)
    transform GetRelative(const transform& child);

    static transform FromMat4(const Math::mat4& mat);

    Math::vec3 position;
    Math::quat rotation;
    Math::vec3 scale;    
};

//------------------------------------------------------------------------------
/**
*/
inline transform::transform() : position(0.0f), scale(1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline transform::transform(const Math::vec3& pos, const Math::quat& rot, const Math::vec3& inScale) :
    position(pos),
    rotation(rot),
    scale(inScale)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform::Set(const Math::vec3& pos, const Math::quat& rot, const Math::vec3& inScale)
{
    this->rotation = rot;
    this->position = pos;
    this->position = inScale;
}


//------------------------------------------------------------------------------
/**
*/
inline transform
transform::GetRelative(const transform& child)
{
    // this has many weird edge cases with scale, esp. negative
    n_assert2(greater_all(child.scale, _zero) && greater_all(scale, _zero), "transform currently does not support negative scale");
    transform result;
    const vec3 invChildScale = reciprocal(child.scale);
    result.scale = multiply(scale, invChildScale);
    quat invChildRot = inverse(child.rotation);
    result.rotation = rotation * invChildRot;
    result.position = multiply(rotate(result.rotation, (position - child.position)), invChildScale);
    return result;
}

//------------------------------------------------------------------------------
/**
*/
inline transform
operator*(const transform& a, const transform& b)
{
    // this has many weird edge cases with scale, esp. negative
    n_assert2(greater_all(a.scale, _zero) && greater_all(b.scale, _zero), "transform currently does not support negative scale");
    transform result;
    result.rotation = a.rotation * b.rotation;
    result.scale = multiply(a.scale, b.scale);
    result.position = rotate(b.rotation, multiply(b.scale, a.position)) + b.position;
    return result;
}

//------------------------------------------------------------------------------
/**
*/
inline transform
lerp(const transform& a, const transform& b, float t)
{
    return transform(lerp(a.position, b.position, t), normalize(slerp(a.rotation, b.rotation, t)), lerp(a.scale, b.scale, t));
}

//------------------------------------------------------------------------------
/** transforms a position (applies scale, rotation and offset)
*/
inline point
operator*(const transform& a, const point& b)
{
    return rotate(a.rotation, multiply(a.scale, b.vec)) + a.position;
}

//------------------------------------------------------------------------------
/** transforms a vector (ignores position)
*/
inline vector
operator*(const transform& a, const vector& b)
{
    return rotate(a.rotation, multiply(a.scale, b.vec));
}


//------------------------------------------------------------------------------
/** transforms a vector (ignores position)
*/
inline transform
transform::FromMat4(const Math::mat4& mat)
{
    transform trans;
    Math::decompose(mat, trans.scale, trans.rotation, trans.position);
    return trans;
}

}
