#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::Transform
    
    Transform based on an __mm256 which stores an quaternion and position.
    
    (C) 2017 Individual contributors, see AUTHORS file
*/

#include "core/types.h"
#include "math/matrix44.h"
#include "math/point.h"
#include "math/quaternion.h"

//------------------------------------------------------------------------------
namespace Math
{
// it appears that this is not required according to vs2015
//NEBULA_ALIGN16
class transform
{
public:
    /// default constructor
    transform();
    /// copy constructor
    transform(const __m256 & other);
    /// constructor using quat&position
    transform(const quaternion & quat, const point & position);
    /// convert to matrix
    void to_matrix44(matrix44 & target) const;    
    /// assignment
    void operator= (const transform & other);
    /// update
    void set(const quaternion &quat, const point & position);
    /// get position
    point get_position() const;
    /// get orientation
    quaternion get_orientation() const;

private:
    __m256 trans;
}; 


//------------------------------------------------------------------------------
/**
*/
__forceinline
transform::transform()
{
    __m128 quat = DirectX::XMQuaternionIdentity();
    __m128 zero = _mm_setzero_ps();
    this->trans = _mm256_set_m128(quat, zero);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline
transform::transform(const __m256 & other):trans(other)
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
__forceinline
transform::transform(const quaternion & quat, const point & position)
{
    this->trans = _mm256_set_m128(quat.vec, position.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
transform::to_matrix44(matrix44 & target) const
{
    __m256 loc = this->trans;
    DirectX::XMMATRIX mat = DirectX::XMMatrixRotationQuaternion(_mm256_extractf128_ps(loc, 1));
    mat.r[3] = _mm256_extractf128_ps(loc, 0);
    target = mat;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void
transform::set(const quaternion &quat, const point & position)
{
    this->trans = _mm256_set_m128(quat.vec, position.vec);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline Math::point
transform::get_position() const
{
    return _mm256_extractf128_ps(this->trans, 0);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline Math::quaternion
transform::get_orientation() const
{
    return _mm256_extractf128_ps(this->trans, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline  void
transform::operator=(const transform & other)
{
    this->trans = other.trans;
}

} // namespace Math
//------------------------------------------------------------------------------