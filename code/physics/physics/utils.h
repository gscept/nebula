#pragma once
//------------------------------------------------------------------------------
/**
    PhysX utils for conversion of datatypes

    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/

#include "PxConfig.h"
#include "characterkinematic/PxExtended.h"
#include "foundation/PxVec3.h"
#include "math/vector.h"
#include "foundation/PxVec4.h"
#include "foundation/PxMat44.h"
#include "math/transform.h"

//------------------------------------------------------------------------------
/**
*/
inline physx::PxVec3
Neb2PxVec(const Math::vector& vec)
{
    return physx::PxVec3(vec.x, vec.y, vec.z);
}

//--------------------------------------------------------------------------
/**
*/
inline physx::PxExtendedVec3
Neb2PxExtentedVec3(const Math::vec3& vec)
{
    return physx::PxExtendedVec3(vec.x, vec.y, vec.z);
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxVec3
Neb2PxPnt(const Math::point& vec)
{
    return physx::PxVec3(vec.x, vec.y, vec.z);
}

//------------------------------------------------------------------------------
/**
*/
inline  Math::vector
Px2NebVec(const physx::PxVec3& vec)
{
    return Math::vector(vec.x, vec.y, vec.z);
}

//------------------------------------------------------------------------------
/**
*/
inline  Math::vec4
Px2Nebfloat4(const physx::PxVec4& vec)
{
    return Math::vec4(vec.x, vec.y, vec.z, vec.w);
}

//------------------------------------------------------------------------------
/**
*/
inline  Math::point
Px2NebPoint(const physx::PxVec3& vec)
{
    return Math::point(vec.x, vec.y, vec.z);
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxVec4
Neb2PxVec4(const Math::vec4& vec)
{
    return physx::PxVec4(vec.x, vec.y, vec.z, vec.w);
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxQuat
Neb2PxQuat(const Math::quat& vec)
{
    return physx::PxQuat(vec.x, vec.y, vec.z, vec.w);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::quat
Px2NebQuat(const physx::PxQuat& vec)
{
    return Math::quat(vec.x, vec.y, vec.z, vec.w);
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxMat44
Neb2PxMat(const Math::mat4& mat)
{
    return physx::PxMat44(Neb2PxVec4(mat.row0), Neb2PxVec4(mat.row1), Neb2PxVec4(mat.row2), Neb2PxVec4(mat.row3));
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxTransform
Neb2PxTrans(const Math::mat4& mat)
{
    return physx::PxTransform(Neb2PxMat(mat));
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxTransform
Neb2PxTrans(const Math::vec3& position, const Math::quat& orientation)
{
    return physx::PxTransform(Neb2PxVec(position), Neb2PxQuat(orientation));
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxTransform
Neb2PxTrans(const Math::transform& trans)
{
    n_assert(trans.scale == Math::_plus1);
    return physx::PxTransform(Neb2PxVec(trans.position), Neb2PxQuat(trans.rotation));
}

inline Math::transform
Px2NebTrans(const physx::PxTransform& trans)
{
    return Math::transform(Px2NebVec(trans.p), Px2NebQuat(trans.q));
}

//------------------------------------------------------------------------------
/**
*/
inline Math::mat4
Px2NebMat(const physx::PxTransform& mat)
{
    Math::quat q = Px2NebQuat(mat.q);
    Math::vec4 p = Px2NebPoint(mat.p);
    Math::mat4 m = Math::rotationquat(q);
    m.position = p;
    return m;
}
