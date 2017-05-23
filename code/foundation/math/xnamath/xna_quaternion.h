#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::quaternion
  
    A quaternion class on top of the Xbox360 math functions.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file
*/    
#include "core/types.h"
#include "math/scalar.h"
#include "math/float4.h"

//------------------------------------------------------------------------------
namespace Math
{
class quaternion;

#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
typedef const quaternion  __QuaternionArg;
#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const quaternion& __QuaternionArg;
#endif


_DECLSPEC_ALIGN_16_ 
#if __XBOX360__
__declspec(passinreg)
#endif
class quaternion
{
public:
    /// default constructor, NOTE: does NOT setup components!
    quaternion();
    /// construct from components
    quaternion(scalar x, scalar y, scalar z, scalar w);
    /// construct from float4
    quaternion(float4 const &rhs);
    /// copy constructor
    /// !!!! copy constructor forbidden, otherwise passing point's to a function
    /// !!!! via Registers doesnt work
    //quaternion(const quaternion& rhs);
    /// construct from XMVECTOR
    quaternion(XMVECTOR rhs);

    /// assignment operator
    void operator=(const quaternion& rhs);
    /// assign XMVECTOR
    void operator=(XMVECTOR rhs);
    /// equality operator
    bool operator==(const quaternion& rhs) const;
    /// inequality operator
    bool operator!=(const quaternion& rhs) const;

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// stream content to 16-byte-aligned memory circumventing the write-cache
    void stream(scalar* ptr) const;

    /// set content
    void set(scalar x, scalar y, scalar z, scalar w);
    /// set from float4
    void set(float4 const &f4);
    /// set the x component
    void set_x(scalar x);
    /// set the y component
    void set_y(scalar y);
    /// set the z component
    void set_z(scalar z);
    /// set the w component
    void set_w(scalar w);

    /// read/write access to x component
    scalar& x();
    /// read/write access to y component
    scalar& y();
    /// read/write access to z component
    scalar& z();
    /// read/write access to w component
    scalar& w();
    /// read-only access to x component
    scalar x() const;
    /// read-only access to y component
    scalar y() const;
    /// read-only access to z component
    scalar z() const;
    /// read-only access to w component
    scalar w() const;
    
    /// return true if quaternion is identity
    bool isidentity() const;
    /// returns length
    scalar length() const;
    /// returns length squared
    scalar lengthsq() const;
    /// un-denormalize quaternion (this is sort of a hack since Maya likes to return denormal quaternions)
    void undenormalize();

    /// return quaternion in barycentric coordinates
    static quaternion barycentric(const quaternion& q0, const quaternion& q1, const quaternion& q2, scalar f, scalar g);
    /// return conjugate of a normalized quaternion
    static quaternion conjugate(const quaternion& q);
    /// return dot product of two normalized quaternions
    static scalar dot(const quaternion& q0, const quaternion& q1);
    /// calculate the exponential
    static quaternion exp(const quaternion& q0);
    /// returns an identity quaternion
    static quaternion identity();
    /// conjugates and renormalizes quaternion
    static quaternion inverse(const quaternion& q);
    /// calculate the natural logarithm
    static quaternion ln(const quaternion& q);
    /// multiply 2 quaternions
    static quaternion multiply(const quaternion& q0, const quaternion& q1);
    /// compute unit length quaternion
    static quaternion normalize(const quaternion& q);
    /// build quaternion from axis and clockwise rotation angle in radians
    static quaternion rotationaxis(const float4& axis, scalar angle);
    /// build quaternion from rotation matrix
    static quaternion rotationmatrix(const matrix44& m);
    /// build quaternion from yaw, pitch and roll
    static quaternion rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll);
    /// interpolate between 2 quaternion using spherical interpolation
    static quaternion slerp(const quaternion& q1, const quaternion& q2, scalar t);
    /// setup control points for spherical quadrangle interpolation
    static void squadsetup(const quaternion& q0, const quaternion& q1, const quaternion& q2, const quaternion& q3, quaternion& aOut, quaternion& bOut, quaternion& cOut);
    /// interpolate between quaternions using spherical quadrangle interpolation
    static quaternion squad(const quaternion& q1, const quaternion& a, const quaternion& b, const quaternion& c, scalar t);
    /// convert quaternion to axis and angle
    static void to_axisangle(const quaternion& q, float4& outAxis, scalar& outAngle);

private:
    friend class matrix44;

    XMVECTOR vec;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = XMVectorSet(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion(float4 const &rhs) :
    vec(rhs.vec)
{
    // empty
}
    
//------------------------------------------------------------------------------
/**
*/
__forceinline 
quaternion::quaternion(XMVECTOR rhs) :
    vec(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::operator=(const quaternion& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::operator=(XMVECTOR rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator==(const quaternion& rhs) const
{
    return (0 != XMQuaternionEqual(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator!=(const quaternion& rhs) const
{
    return (0 != XMQuaternionNotEqual(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::load(const scalar* ptr)
{
    this->vec = XMLoadFloat4A((XMFLOAT4A*)ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::loadu(const scalar* ptr)
{
    this->vec = XMLoadFloat4((XMFLOAT4*)ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::store(scalar* ptr) const
{
    XMStoreFloat4A((XMFLOAT4A*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::storeu(scalar* ptr) const
{
    XMStoreFloat4((XMFLOAT4*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::stream(scalar* ptr) const
{
    XMStoreFloat4A((XMFLOAT4A*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::set(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = XMVectorSet(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
quaternion::set_x(scalar x)
{
    this->vec = XMVectorSetXPtr(this->vec, &x);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
quaternion::set_y(scalar y)
{
    this->vec = XMVectorSetYPtr(this->vec, &y);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
quaternion::set_z(scalar z)
{
    this->vec = XMVectorSetZPtr(this->vec, &z);
}

//------------------------------------------------------------------------------
/**
*/
inline void 
quaternion::set_w(scalar w)
{
    this->vec = XMVectorSetWPtr(this->vec, &w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::set(float4 const &f4)
{
    this->vec = f4.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::x()
{
#if __XBOX360__ || defined(_XM_NO_INTRINSICS_)
    return this->vec.x;
#elif __WIN32__
    return this->vec.m128_f32[0];
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::x() const
{
    return float4::unpack_x(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::y()
{
#if __XBOX360__ || defined(_XM_NO_INTRINSICS_)
    return this->vec.y;
#elif __WIN32__
    return this->vec.m128_f32[1];
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::y() const
{
    return float4::unpack_y(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::z()
{
#if __XBOX360__ || defined(_XM_NO_INTRINSICS_)
    return this->vec.z;
#elif __WIN32__
    return this->vec.m128_f32[2];
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::z() const
{
    return float4::unpack_z(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::w()
{
#if __XBOX360__ || defined(_XM_NO_INTRINSICS_)
    return this->vec.w;
#elif __WIN32__
    return this->vec.m128_f32[3];
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::w() const
{
    return float4::unpack_w(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::isidentity() const
{
    return (0 != XMQuaternionIsIdentity(this->vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::length() const
{
    return float4::unpack_x(XMQuaternionLength(this->vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::lengthsq() const
{
    return float4::unpack_x(XMQuaternionLengthSq(this->vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::undenormalize()
{
    // nothing to do on the xbox, since denormal numbers are not supported by the vmx unit, 
    // it is being set to zero anyway
#if __WIN32__
    this->set_x(n_undenormalize(this->x()));
    this->set_y(n_undenormalize(this->y()));
    this->set_z(n_undenormalize(this->z()));
    this->set_w(n_undenormalize(this->w()));
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::barycentric(const quaternion& q0, const quaternion& q1, const quaternion& q2, scalar f, scalar g)
{
    return XMQuaternionBaryCentric(q0.vec, q1.vec, q2.vec, f, g);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::conjugate(const quaternion& q)
{
    return XMQuaternionConjugate(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::dot(const quaternion& q0, const quaternion& q1)
{
    return float4::unpack_x(XMQuaternionDot(q0.vec, q1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::exp(const quaternion& q)
{
    return XMQuaternionExp(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::identity()
{
    return XMQuaternionIdentity();
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::inverse(const quaternion& q)
{
    return XMQuaternionInverse(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::ln(const quaternion& q)
{
    return XMQuaternionLn(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::multiply(const quaternion& q0, const quaternion& q1)
{
    return XMQuaternionMultiply(q0.vec, q1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::normalize(const quaternion& q)
{
    return XMQuaternionNormalize(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::rotationaxis(const float4& axis, scalar angle)
{
    return XMQuaternionRotationAxis(axis.vec, angle);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll)
{
    return XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::slerp(const quaternion& q1, const quaternion& q2, scalar t)
{
    return XMQuaternionSlerp(q1.vec, q2.vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::squadsetup(const quaternion& q0, const quaternion& q1, const quaternion& q2, const quaternion& q3, quaternion& aOut, quaternion& bOut, quaternion& cOut)
{
    XMQuaternionSquadSetup(&aOut.vec, &bOut.vec, &cOut.vec, q0.vec, q1.vec, q2.vec, q3.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::squad(const quaternion& q1, const quaternion& a, const quaternion& b, const quaternion& c, scalar t)
{
    return XMQuaternionSquad(q1.vec, a.vec, b.vec, c.vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::to_axisangle(const quaternion& q, float4& outAxis, scalar& outAngle)
{
    XMQuaternionToAxisAngle(&outAxis.vec, &outAngle, q.vec);
    outAxis.set_w(0.0f);
}

} // namespace Math
//------------------------------------------------------------------------------
