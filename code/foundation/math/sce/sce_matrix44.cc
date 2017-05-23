//------------------------------------------------------------------------------
//  matrix44.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "math/sce/sce_matrix44.h"
#include "math/sce/sce_plane.h"
// XXX: #include "math/sce/sce_quaternion.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::reflect(const plane& p)
{
    matrix44 res;
    // XXX: D3DXMatrixReflect((D3DXMATRIX*)&res, (CONST D3DXPLANE*)&p);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void
matrix44::decompose(float4& outScale, quaternion& outRotation, float4& outTranslation) const
{
    outScale.set(0.0f, 0.0f, 0.0f, 0.0f);
    outTranslation.set(0.0f, 0.0f, 0.0f, 0.0f);
    // XXX: D3DXMatrixDecompose((D3DXVECTOR3*)&outScale, (D3DXQUATERNION*)&outRotation, (D3DXVECTOR3*)&outTranslation, (CONST D3DXMATRIX*)this);
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::affinetransformation(scalar scaling, const float4& rotationCenter, const quaternion& rotation, const float4& translation)
{
    matrix44 res;
    // XXX: D3DXMatrixAffineTransformation((D3DXMATRIX*)&res, scaling, (CONST D3DXVECTOR3*)&rotationCenter, (CONST D3DXQUATERNION*)&rotation, (CONST D3DXVECTOR3*)&translation);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::rotationquaternion(const quaternion& q)
{
    matrix44 res;
    // XXX: D3DXMatrixRotationQuaternion((D3DXMATRIX*)&res, (CONST D3DXQUATERNION*)&q);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::transformation(const float4& scalingCenter, const quaternion& scalingRotation, const float4& scaling, const float4& rotationCenter, const quaternion& rotation, const float4& translation)
{
    matrix44 res;
#if 0
    D3DXMatrixTransformation((D3DXMATRIX*)&res, 
                             (CONST D3DXVECTOR3*)&scalingCenter, 
                             (CONST D3DXQUATERNION*)&scalingRotation, 
                             (CONST D3DXVECTOR3*)&scaling, 
                             (CONST D3DXVECTOR3*)&rotationCenter, 
                             (CONST D3DXQUATERNION*)&rotation, 
                             (CONST D3DXVECTOR3*)&translation);
#endif
    return res;
}

//------------------------------------------------------------------------------
/**
*/
float4 
Math::matrix44::transform(const float4& v, const Math::matrix44& m)
{
    float4 res;
    // D3DXMatrixMultiply
    return res;
}

//------------------------------------------------------------------------------
/**
*/
Math::plane 
Math::matrix44::transform(const Math::plane& p, const Math::matrix44& m)
{
    plane res;
    // D3DXMatrixMultiply
    return res;
}

} // namespace Math
