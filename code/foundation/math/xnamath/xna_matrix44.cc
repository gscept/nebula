//------------------------------------------------------------------------------
//  matrix44.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2014 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "math/matrix44.h"
#include "math/plane.h"
#include "math/quaternion.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::reflect(const plane& p)
{
    return XMMatrixReflect(p.vec);
}

//------------------------------------------------------------------------------
/**
*/
bool
matrix44::decompose(float4& outScale, quaternion& outRotation, float4& outTranslation) const
{
    BOOL result = XMMatrixDecompose(&outScale.vec, 
                        &outRotation.vec, 
                        &outTranslation.vec, 
                        this->mx);
    outScale.set_w(0.0f);
    outTranslation.set_w(0.0f);
	return result == TRUE;
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::affinetransformation(scalar scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation)
{
    return XMMatrixAffineTransformation(XMVectorReplicate(scaling), rotationCenter.vec, rotation.vec, translation.vec);
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::rotationquaternion(const quaternion& q)
{
    return XMMatrixRotationQuaternion(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
matrix44
matrix44::transformation(float4 const &scalingCenter, const quaternion& scalingRotation, float4 const &scaling, float4 const &rotationCenter, const quaternion& rotation, float4 const &translation)
{
    return XMMatrixTransformation(scalingCenter.vec,
                                  scalingRotation.vec,
                                  scaling.vec,
                                  rotationCenter.vec,
                                  rotation.vec,
                                  translation.vec);
}

//------------------------------------------------------------------------------
/**
*/
bool 
matrix44::ispointinside(const float4& p, const matrix44& m)
{
    float4 p1 = matrix44::transform(p, m);
    // vectorized compare operation
    return !(float4::less4_any(float4(p1.x(), p1.w(), p1.y(), p1.w()), 
             float4(-p1.w(), p1.x(), -p1.w(), p1.y()))
            ||
            float4::less4_any(float4(p1.z(), p1.w(), 0, 0), 
            float4(-p1.w(), p1.z(), 0, 0)));
}
} // namespace Math
