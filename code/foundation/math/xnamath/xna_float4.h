#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::float4

    Xbox360 implementation of float4. Note: float4 params are handed
    as values on the Xbox360, since 4 component vectors are a native datatype
    (and the compiler knows how to handle this stuff).

    (C) 2007 Radon Labs GmbH
    (C) 2013-2014 Individual contributors, see AUTHORS file	
*/
#include "core/types.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace Math
{
class matrix44;
class float4;

#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
typedef const float4 __Float4Arg;
#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const float4& __Float4Arg;
#endif


_DECLSPEC_ALIGN_16_
#if __XBOX360__
__declspec(passinreg)
#endif
class float4
{
public:
    /// default constructor, NOTE: does NOT setup components!
    float4();
    /// construct from values
    float4(scalar x, scalar y, scalar z, scalar w);
    /// construct from single value which sets all components to value
    float4(scalar v);
    /// !!!! copy constructor forbidden, otherwise passing float4's to a function
    /// !!!! via Registers doesnt work
    //float4(const float4& rhs);
    /// construct from XMVECTOR
    float4(XMVECTOR rhs);

    /// assignment operator
    void operator=(const float4 &rhs);
    /// assign an XMVECTOR
    void operator=(XMVECTOR rhs);
    /// flip sign
    float4 operator-() const;
    /// inplace add
    void operator+=(const float4 &rhs);
    /// inplace sub
    void operator-=(const float4 &rhs);
    /// inplace scalar multiply
    void operator*=(scalar s);
    /// muliply by a vector component-wise
    void operator*=(const float4& rhs);
	/// divide by a vector component-wise
	void operator/=(const float4& rhs);
    /// add 2 vectors
    float4 operator+(const float4 &rhs) const;
    /// subtract 2 vectors
    float4 operator-(const float4 &rhs) const;
    /// multiply with scalar
    float4 operator*(scalar s) const;
    /// equality operator
    bool operator==(const float4 &rhs) const;
    /// inequality operator
    bool operator!=(const float4 &rhs) const;

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

    /// load 3 floats into x,y,z from unaligned memory
    void load_float3(const void* ptr, float w);
    /// load from UByte4N packed vector, move range to -1..+1
    void load_ubyte4n_signed(const void* ptr, float w);

    /// set content
    void set(scalar x, scalar y, scalar z, scalar w);
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
	/// read/write access to indexed component
	scalar& operator[](const int index);
    /// return length of vector
    scalar length() const;
	/// return length of vectors XYZ components
	scalar length3() const;
    /// return squared length of vector
    scalar lengthsq() const;
	/// return squared length of vectors XYZ components
	scalar lengthsq3() const;
    /// return component-wise absolute
    float4 abs() const;

    /// return 1.0 / vec
    static float4 reciprocal(const float4 &v);
    /// component-wise multiplication
    static float4 multiply(const float4 &v0, const float4 &v1);
	/// component-wise multiply and add
	static float4 multiplyadd(const float4 &v0, const float4 &v1, const float4 &v2);
    /// component wise division
    static float4 divide(const float4& v0, const float4 &v1);
    /// return 3-dimensional cross product
    static float4 cross3(const float4 &v0, const float4 &v1);
    /// return 3d dot product of vectors
    static scalar dot3(const float4 &v0, const float4 &v1);
    /// return point in barycentric coordinates
    static float4 barycentric(const float4 &v0, const float4 &v1, const float4 &v2, scalar f, scalar g);
    /// perform Catmull-Rom interpolation
    static float4 catmullrom(const float4 &v0, const float4 &v1, const float4 &v2, const float4 &v3, scalar s);
    /// perform Hermite spline interpolation
    static float4 hermite(const float4 &v1, const float4 &t1, const float4 &v2, const float4 &t2, scalar s);
    /// perform linear interpolation between 2 4d vectors
    static float4 lerp(const float4 &v0, const float4 &v1, scalar s);
    /// return 4d vector made up of largest components of 2 vectors
    static float4 maximize(const float4 &v0, const float4 &v1);
    /// return 4d vector made up of smallest components of 2 vectors
    static float4 minimize(const float4 &v0, const float4 &v1);
    /// return normalized version of 4d vector
    static float4 normalize(const float4 &v);
    /// transform 4d vector by matrix44
    static __declspec(deprecated) float4 transform(__Float4Arg v, const matrix44 &m);
    /// reflect a vector v
    static float4 reflect(const float4 &normal, const float4 &incident);
    /// clamp to min/max vector
    static float4 clamp(__Float4Arg Clamp, __Float4Arg vMin, __Float4Arg vMax);
    /// angle between two vectors
    static scalar angle(__Float4Arg v0, __Float4Arg v1);
	/// returns vector of boolean values where the values of v1 or v2 corresponds to control
	static float4 select(const float4& v0, const float4& v1, const float4& control);
	/// returns a zero vector
	static float4 zerovector();

    /// return true if any XYZ component is less-then
    static bool less3_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZ components are less-then
    static bool less3_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZ component is less-or-equal
    static bool lessequal3_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZ components are less-or-equal
    static bool lessequal3_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZ component is greater
    static bool greater3_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZ components are greater
    static bool greater3_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZ component is greater-or-equal
    static bool greaterequal3_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZ components are greater-or-equal
    static bool greaterequal3_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZ component is equal
    static bool equal3_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZ components are equal
    static bool equal3_all(const float4 &v0, const float4 &v1);
    /// perform near equal comparison with given epsilon (3 components)
    static bool nearequal3(const float4 &v0, const float4 &v1, const float4 &epsilon);

    /// return true if any XYZW component is less-then
    static bool less4_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZW components are less-then
    static bool less4_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZW component is less-or-equal
    static bool lessequal4_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZW components are less-or-equal
    static bool lessequal4_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZW component is greater
    static bool greater4_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZW components are greater
    static bool greater4_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZW component is greater-or-equal
    static bool greaterequal4_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZW components are greater-or-equal
    static bool greaterequal4_all(const float4 &v0, const float4 &v1);
    /// return true if any XYZW component is equal
    static bool equal4_any(const float4 &v0, const float4 &v1);
    /// return true if all XYZW components are equal
    static bool equal4_all(const float4 &v0, const float4 &v1);
    /// perform near equal comparison with given epsilon (4 components)
    static bool nearequal4(const float4 &v0, const float4 &v1, const float4 &epsilon);

    /// perform less of XYZ and returns vector with results
    static float4 less(const float4& v0, const float4& v1);
    /// perform greater of XYZ and returns vector with results
    static float4 greater(const float4& v0, const float4& v1);
    /// perform equal of XYZ and returns vector with results
    static float4 equal(const float4& v0, const float4& v1);

    /// unpack the first element from a XMVECTOR
    static float unpack_x(XMVECTOR v);
    /// unpack the second element from a XMVECTOR
    static float unpack_y(XMVECTOR v);
    /// unpack the third element from a XMVECTOR
    static float unpack_z(XMVECTOR v);
    /// unpack the fourth element from a XMVECTOR
    static float unpack_w(XMVECTOR v);
    /// splat scalar into each component of a vector
    static float4 splat(scalar s);
    /// return a vector with all elements set to element n of v. 0 <= element <= 3
    static float4 splat(const float4 &v, uint element);
    /// return a vector with all elements set to v.x
    static float4 splat_x(const float4 &v);
    /// return a vector with all elements set to v.y
    static float4 splat_y(const float4 &v);
    /// return a vector with all elements set to v.z
    static float4 splat_z(const float4 &v);
    /// return a vector with all elements set to v.w
    static float4 splat_w(const float4 &v);        
    /// merge components of 2 vectors into 1 (see XMVectorPermute for details)
    static float4 permute(const float4& v0, const float4& v1, unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3);
	/// floor
	static float4 floor(const float4 &v);
	/// ceil
	static float4 ceiling(const float4 &v);

    XMVECTOR vec;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4()
{
    //  empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = XMVectorSet(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(scalar v)
{
    this->vec = XMVectorSet(v, v, v, v);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(XMVECTOR rhs) :
    vec(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator=(const float4 &rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator=(XMVECTOR rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::operator==(const float4 &rhs) const
{
    return (0 != XMVector4Equal(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::operator!=(const float4 &rhs) const
{
    return (0 != XMVector4NotEqual(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
float4::load(const scalar* ptr)
{
    this->vec = XMLoadFloat4A((XMFLOAT4A*)ptr);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
float4::loadu(const scalar* ptr)
{
    this->vec = XMLoadFloat4((XMFLOAT4*)ptr);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
float4::store(scalar* ptr) const
{
    XMStoreFloat4A((XMFLOAT4A*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
float4::storeu(scalar* ptr) const
{
    XMStoreFloat4((XMFLOAT4*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::stream(scalar* ptr) const
{
    XMStoreFloat4A((XMFLOAT4A*)ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::load_float3(const void* ptr, float w)
{
    this->vec = XMLoadFloat3((_XMFLOAT3*)ptr);
    this->set_w(w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator-() const
{
    return XMVectorNegate(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator*(scalar t) const
{
    return XMVectorScale(this->vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator*=(const float4& rhs)
{
    this->vec = XMVectorMultiply(this->vec, rhs.vec);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void 
float4::operator/=( const float4& rhs )
{
    this->vec = XMVectorDivide(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator+=(const float4 &rhs)
{
    this->vec = XMVectorAdd(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator-=(const float4 &rhs)
{
    this->vec = XMVectorSubtract(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator*=(scalar s)
{
    this->vec = XMVectorScale(this->vec, s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator+(const float4 &rhs) const
{
    return XMVectorAdd(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator-(const float4 &rhs) const
{
    return XMVectorSubtract(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = XMVectorSet(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::x()
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
float4::x() const
{
    return float4::unpack_x(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::y()
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
float4::y() const
{
    return float4::unpack_y(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::z()
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
float4::z() const
{
    return float4::unpack_z(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::w()
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
float4::w() const
{
    return float4::unpack_w(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::operator[]( const int index )
{
	n_assert(index < 4);
	return this->vec.m128_f32[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set_x(scalar x)
{
    this->vec = XMVectorSetXPtr(this->vec, &x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set_y(scalar y)
{
    this->vec = XMVectorSetYPtr(this->vec, &y);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set_z(scalar z)
{
    this->vec = XMVectorSetZPtr(this->vec, &z);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set_w(scalar w)
{
    this->vec = XMVectorSetWPtr(this->vec, &w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::length() const
{
    return float4::unpack_x(XMVector4Length(this->vec));
}
//------------------------------------------------------------------------------
/**
*/
__forceinline scalar 
float4::length3() const
{
	return float4::unpack_x(XMVector3Length(this->vec));
}


//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::lengthsq() const
{
    return float4::unpack_x(XMVector4LengthSq(this->vec));
}


//------------------------------------------------------------------------------
/**
*/
__forceinline scalar 
float4::lengthsq3() const
{
	return float4::unpack_x(XMVector3LengthSq(this->vec));
}
//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::reciprocal(const float4 &v)
{
    return XMVectorReciprocal(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::multiply(const float4 &v0, const float4 &v1)
{
    return XMVectorMultiply(v0.vec, v1.vec);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::multiplyadd( const float4 &v0, const float4 &v1, const float4 &v2 )
{
	return XMVectorMultiplyAdd(v0.vec, v1.vec, v2.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::divide(const float4& v0, const float4 &v1)
{
    return XMVectorDivide(v0.vec, v1.vec);    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::abs() const
{
    return XMVectorAbs(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::cross3(const float4 &v0, const float4 &v1)
{
    return XMVector3Cross(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::dot3(const float4 &v0, const float4 &v1)
{
    return float4::unpack_x(XMVector3Dot(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::barycentric(const float4 &v0, const float4 &v1, const float4 &v2, scalar f, scalar g)
{
    return XMVectorBaryCentric(v0.vec, v1.vec, v2.vec, f, g);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::catmullrom(const float4 &v0, const float4 &v1, const float4 &v2, const float4 &v3, scalar s)
{
    return XMVectorCatmullRom(v0.vec, v1.vec, v2.vec, v3.vec, s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::hermite(const float4 &v1, const float4 &t1, const float4 &v2, const float4 &t2, scalar s)
{
    return XMVectorHermite(v1.vec, t1.vec, v2.vec, t2.vec, s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::lerp(const float4 &v0, const float4 &v1, scalar s)
{
    return XMVectorLerp(v0.vec, v1.vec, s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::maximize(const float4 &v0, const float4 &v1)
{
    return XMVectorMax(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::minimize(const float4 &v0, const float4 &v1)
{
    return XMVectorMin(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::normalize(const float4 &v)
{
    if (float4::equal3_all(v, float4(0,0,0,0))) return v;
    return XMVector4Normalize(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::reflect(const float4 &normal, const float4 &incident)
{
    return XMVector3Reflect(incident.vec, normal.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less4_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyFalse(XMVector4GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less4_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllFalse(XMVector4GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal4_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyFalse(XMVector4GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal4_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllFalse(XMVector4GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater4_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector4GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater4_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector4GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal4_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector4GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal4_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector4GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal4_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector4EqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal4_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector4EqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::nearequal4(const float4 &v0, const float4 &v1, const float4 &epsilon)
{
    return (0 != XMVector4NearEqual(v0.vec, v1.vec, epsilon.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less3_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyFalse(XMVector3GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less3_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllFalse(XMVector3GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal3_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyFalse(XMVector3GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal3_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllFalse(XMVector3GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater3_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector3GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater3_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector3GreaterR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal3_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector3GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal3_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector3GreaterOrEqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal3_any(const float4 &v0, const float4 &v1)
{
    return XMComparisonAnyTrue(XMVector3EqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal3_all(const float4 &v0, const float4 &v1)
{
    return XMComparisonAllTrue(XMVector3EqualR(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::nearequal3(const float4 &v0, const float4 &v1, const float4 &epsilon)
{
    return (0 != XMVector3NearEqual(v0.vec, v1.vec, epsilon.vec));
}

//------------------------------------------------------------------------------
/**
    Clamp between 0-1
*/
__forceinline Math::float4 
float4::less( const float4& v0, const float4& v1 )
{
    return XMVectorMin(XMVectorLess(v0.vec, v1.vec), XMVectorSplatOne());
}

//------------------------------------------------------------------------------
/**
    Clamp between 0-1
*/
__forceinline Math::float4 
float4::greater( const float4& v0, const float4& v1 )
{
    return XMVectorMin(XMVectorGreater(v0.vec, v1.vec), XMVectorSplatOne());
}

//------------------------------------------------------------------------------
/**
    Clamp between 0-1
*/
__forceinline Math::float4 
float4::equal( const float4& v0, const float4& v1 )
{
    return XMVectorMin(XMVectorEqual(v0.vec, v1.vec), XMVectorSplatOne());
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_x(XMVECTOR v)
{
    FLOAT x;
    XMVectorGetXPtr(&x, v);
    return x;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_y(XMVECTOR v)
{
    FLOAT y;
    XMVectorGetYPtr(&y, v);
    return y;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_z(XMVECTOR v)
{
    FLOAT z;
    XMVectorGetZPtr(&z, v);
    return z;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_w(XMVECTOR v)
{
    FLOAT w;
    XMVectorGetWPtr(&w, v);
    return w;
}
//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat(scalar s)
{
	XMVECTOR v;
	v = XMVectorSetX(v, s);
	return float4(XMVectorSplatX(v));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat(const float4 &v, uint element)
{
    n_assert(element < 4);
    switch(element)
    {
    case 0:
        return float4(XMVectorSplatX(v.vec));
    case 1:
        return float4(XMVectorSplatY(v.vec));
    case 2:
        return float4(XMVectorSplatZ(v.vec));
    }
    return float4(XMVectorSplatW(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_x(const float4 &v)
{
    return float4(XMVectorSplatX(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_y(const float4 &v)
{
    return float4(XMVectorSplatY(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_z(const float4 &v)
{
    return float4(XMVectorSplatZ(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_w(const float4 &v)
{
    return float4(XMVectorSplatW(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::permute(const float4& v0, const float4& v1, unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3)
{
    return float4(XMVectorPermute(v0.vec, v1.vec,XMVectorPermuteControl(i0, i1, i2, i3)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::floor(const float4 &v)
{
	return float4(XMVectorFloor(v.vec));
}
//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::ceiling(const float4 &v)
{
	return float4(XMVectorCeiling(v.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::zerovector()
{
	return float4(0, 0, 0, 0);
}

} // namespace Math
//------------------------------------------------------------------------------








