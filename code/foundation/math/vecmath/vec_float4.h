#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::float4

    VectorMath implementation of float4.

    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace Math
{
class matrix44;
class float4;


//#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
//typedef const float4 __Float4Arg;
//#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const float4& __Float4Arg;
//#endif


typedef NEBULA_ALIGN16 struct mm128_vec {
  union {
      float f[4];
      unsigned int u[4];
      __m128 vec;
};
  inline operator __m128() const { return vec;}
} mm128_vec;

 typedef NEBULA_ALIGN16 struct mm128_ivec {
     union {
         int u[4];
         mm128_vec vec;
     };
     inline operator __m128() const { return vec.vec;}
 } mm128_ivec;

 typedef NEBULA_ALIGN16 struct mm128_uivec {
     union {
         unsigned int u[4];
         mm128_vec vec;
     };
     inline operator __m128() const { return vec.vec;}
 } mm128_uivec;
const mm128_vec _id_x =  {1.0f, 0.0f, 0.0f, 0.0f};
const mm128_vec _id_y =  {0.0f, 1.0f, 0.0f, 0.0f};
const mm128_vec _id_z =  {0.0f, 0.0f, 1.0f, 0.0f};
const mm128_vec _id_w =  {0.0f, 0.0f, 0.0f, 1.0f};
const mm128_vec _minus1 = {-1.0f, -1.0f, -1.0f, -1.0f};
const mm128_vec _plus1 = {1.0f, 1.0f, 1.0f, 1.0f};
const mm128_uivec _sign = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
const mm128_uivec _mask_xyz = { 0xFFFFFFFF ,0xFFFFFFFF ,0xFFFFFFFF,0 };




class NEBULA_ALIGN16 float4
{
public:
    /// default constructor, NOTE: does NOT setup components!
    float4();
    /// construct from values
    float4(scalar x, scalar y, scalar z, scalar w);
	/// construct from single value
	float4(scalar v);
	/// copy constructor
    float4(const float4& rhs);
	/// construct from SSE 128 byte float array
	float4(const __m128& rhs);
	/// construct from SSE 128 byte float vector
	float4(const mm128_vec& rhs);

    /// assignment operator
    void operator=(const float4 &rhs);
    /// assign an vmVector4
    void operator=(const __m128 &rhs);
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
    /// load from Byte4N packed vector
    void load_byte4n(const void* ptr, float w);
    /// set content
    void set(scalar x, scalar y, scalar z, scalar w);
    
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
    /// read-only access to indexed component
    scalar& operator[](const int index);
	/// read-only access to indexed component
	scalar operator[](const int index) const;

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
	/// return 1.0 / vec, faster version using approximation
	static float4 reciprocalapprox(const float4 &v);
    /// component-wise multiplication
    static float4 multiply(const float4 &v0, const float4 &v1);
	/// component-wise multiply and add
	static float4 multiplyadd(const float4 &v0, const float4 &v1, const float4 &v2);
    /// component-wise divide
    static float4 divide(const float4 &v0, const float4 &v1);
    /// return 3-dimensional cross product
    static float4 cross3(const float4 &v0, const float4 &v1);
	/// return 4d dot product
    static scalar dot(const float4 &v0, const float4 &v2);
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
	/// return normalized version of 4d vector using faster approximations (introduces small error)
	static float4 normalizeapprox(const float4 &v);
	/// return normalized version of 4d vector, ignoring w
	static float4 normalize3(const float4 &v);
	/// return normalized version of 4d vector using faster approximations (introduces small error), ignoring w
	static float4 normalizeapprox3(const float4 &v);
    /// transform 4d vector by matrix44
    static
    //__declspec(deprecated)
    float4 transform(__Float4Arg v, const matrix44 &m);
    /// reflect a vector v
    static float4 reflect(const float4 &normal, const float4 &incident);
    /// clamp to min/max vector
    static float4 clamp(const float4 & Clamp, const float4 & vMin, const float4 & vMax);
    /// angle between two vectors
    static scalar angle(const float4 & v0, const float4 & v1);
    /// returns vector of boolean values where the values of v1 or v2 corresponds to control
    static float4 select(const float4& v0, const float4& v1, const float4& control);
    /// returns vector of boolean values where the values of v1 or v2 corresponds to control
    static float4 select(const float4& v0, const float4& v1, const uint i0, const uint i1, const uint i2, const uint i3);
    /// returns a zero vector
    static float4 zerovector();
    /// return vector divided by w
    static float4 perspective_div(const float4& v);

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

    /// unpack the first element from a __m128
    static float unpack_x(__m128 v);
    /// unpack the second element from a __m128
    static float unpack_y(__m128 v);
    /// unpack the third element from a __m128
    static float unpack_z(__m128 v);
    /// unpack the fourth element from a __m128
    static float unpack_w(__m128 v);
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
    /// merge components of 2 vectors into 1
    static float4 permute(const float4& v0, const float4& v1, unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3);
    /// floor
    static float4 floor(const float4 &v);
    /// ceil
    static float4 ceiling(const float4 &v);

     mm128_vec vec;
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
    this->vec.vec = _mm_setr_ps(x,y,z,w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(scalar v)
{
    this->vec.vec = _mm_set1_ps(v);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(const __m128 &rhs)
{
    this->vec.vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(const float4 &rhs)
{
    this->vec.vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4::float4(const mm128_vec &rhs) :
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
float4::operator=(const __m128 &rhs)
{
	this->vec.vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::operator==(const float4 &rhs) const
{
	__m128 vTemp = _mm_cmpeq_ps(this->vec,rhs.vec);
	return ((_mm_movemask_ps(vTemp)==0x0f) != 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::operator!=(const float4 &rhs) const
{
	__m128 vTemp = _mm_cmpeq_ps(this->vec,rhs.vec);
	return ((_mm_movemask_ps(vTemp)==0x0f) == 0);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
float4::load(const scalar* ptr)
{
    this->vec.vec = _mm_load_ps(ptr);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
float4::loadu(const scalar* ptr)
{
    this->vec.vec = _mm_loadu_ps(ptr);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
float4::store(scalar* ptr) const
{
    _mm_store_ps(ptr, this->vec.vec);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
float4::storeu(scalar* ptr) const
{
    _mm_storeu_ps(ptr, this->vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::stream(scalar* ptr) const
{
    this->store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::load_float3(const void* ptr, float w)
{
	// FIXME ...
	float * source = (float*)ptr;	
	this->vec.vec = _mm_setr_ps(source[0],source[1],source[2],w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator-() const
{
	return float4( _mm_xor_ps(_sign,this->vec.vec));    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator*(scalar t) const
{
    __m128 temp = _mm_set1_ps(t);
    return _mm_mul_ps(this->vec.vec,temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator*=(const float4& rhs)
{
    this->vec.vec = _mm_mul_ps(this->vec.vec, rhs.vec);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator/=( const float4& rhs )
{
	this->vec.vec = _mm_div_ps(this->vec.vec, rhs.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator+=(const float4 &rhs)
{
    this->vec.vec = _mm_add_ps(this->vec.vec, rhs.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator-=(const float4 &rhs)
{
    this->vec.vec = _mm_sub_ps(this->vec.vec, rhs.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::operator*=(scalar s)
{
    __m128 temp = _mm_set1_ps(s);
    this->vec.vec = _mm_mul_ps(this->vec.vec,temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator+(const float4 &rhs) const
{
    return _mm_add_ps(this->vec.vec, rhs.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::operator-(const float4 &rhs) const
{
    return _mm_sub_ps(this->vec.vec, rhs.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
float4::set(scalar x, scalar y, scalar z, scalar w)
{
	this->vec.vec = _mm_setr_ps(x,y,z,w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::x()
{
	return this->vec.f[0];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::x() const
{
	scalar temp;
	_mm_store_ss(&temp,this->vec.vec);
	return temp;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::y()
{
    return this->vec.f[1];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::y() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec,this->vec.vec,_MM_SHUFFLE(1,1,1,1));
	_mm_store_ss(&ret,temp);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::z()
{
    return this->vec.f[2];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::z() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec,this->vec.vec,_MM_SHUFFLE(2,2,2,2));
	_mm_store_ss(&ret,temp);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::w()
{
    return this->vec.f[3];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::w() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec,this->vec.vec,_MM_SHUFFLE(3,3,3,3));
	_mm_store_ss(&ret,temp);
	return ret;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
float4::operator[]( const int index )
{
	n_assert(index < 4);
	return this->vec.f[index];
}


//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::operator[](const int index) const
{
	n_assert(index < 4);
	return this->vec.f[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::length() const
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(this->vec.vec,this->vec.vec, 0xF1)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::length3() const
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(this->vec.vec,this->vec.vec, 0x71)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::lengthsq() const
{
    return _mm_cvtss_f32(_mm_dp_ps(this->vec.vec,this->vec.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::lengthsq3() const
{
    return _mm_cvtss_f32(_mm_dp_ps(this->vec.vec,this->vec.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::reciprocal(const float4 &v)
{
	return _mm_div_ps(_plus1,v.vec.vec);	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::reciprocalapprox(const float4 &v)
{
    return _mm_rcp_ps(v.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::multiply(const float4 &v0, const float4 &v1)
{
    return _mm_mul_ps(v0.vec.vec, v1.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::multiplyadd( const float4 &v0, const float4 &v1, const float4 &v2 )
{
	return _mm_add_ps(_mm_mul_ps(v0.vec.vec, v1.vec),v2.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4 
float4::divide(const float4 &v0, const float4 &v1)
{
    return _mm_div_ps(v0.vec.vec, v1.vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::abs() const
{
	unsigned int val = 0x7fffffff;
	__m128 temp = _mm_set1_ps(*(float*)&val);
	return _mm_and_ps( this->vec.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::cross3(const float4 &v0, const float4 &v1)
{
	__m128 tmp0, tmp1, tmp2, tmp3, result;
	tmp0 = _mm_shuffle_ps( v0.vec.vec, v0.vec.vec, _MM_SHUFFLE(3,0,2,1) );
	tmp1 = _mm_shuffle_ps( v1.vec.vec, v1.vec.vec, _MM_SHUFFLE(3,1,0,2) );
	tmp2 = _mm_shuffle_ps( v0.vec.vec, v0.vec.vec, _MM_SHUFFLE(3,1,0,2) );
	tmp3 = _mm_shuffle_ps( v1.vec.vec, v1.vec.vec, _MM_SHUFFLE(3,0,2,1) );
	result = _mm_mul_ps( tmp0, tmp1 );
	result = _mm_sub_ps( result, _mm_mul_ps( tmp2, tmp3 ) );
	return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::dot(const float4 &v0, const float4 &v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec.vec,v1.vec.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
float4::dot3(const float4 &v0, const float4 &v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec.vec,v1.vec.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
    calculates Result = v0 + f * (v1 - v0) + g * (v2 - v0)
*/
__forceinline float4
float4::barycentric(const float4 &v0, const float4 &v1, const float4 &v2, scalar f, scalar g)
{
	__m128 R1 = _mm_sub_ps(v1.vec,v0.vec);
	__m128 SF = _mm_set_ps1(f);
	__m128 R2 = _mm_sub_ps(v2.vec,v0.vec);
	__m128 SG = _mm_set_ps1(g);
	R1 = _mm_mul_ps(R1,SF);
	R2 = _mm_mul_ps(R2,SG);
	R1 = _mm_add_ps(R1,v0.vec);
	R1 = _mm_add_ps(R1,R2);
	return R1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::catmullrom(const float4 &v0, const float4 &v1, const float4 &v2, const float4 &v3, scalar s)
{
	scalar s2 = s * s;
	scalar s3 = s * s2;

	__m128 P0 = _mm_set_ps1((-s3 + 2.0f * s2 - s) * 0.5f);
	__m128 P1 = _mm_set_ps1((3.0f * s3 - 5.0f * s2 + 2.0f) * 0.5f);
	__m128 P2 = _mm_set_ps1((-3.0f * s3 + 4.0f * s2 + s) * 0.5f);
	__m128 P3 = _mm_set_ps1((s3 - s2) * 0.5f);

	P0 = _mm_mul_ps(P0, v0.vec);
	P1 = _mm_mul_ps(P1, v1.vec);
	P2 = _mm_mul_ps(P2, v2.vec);
	P3 = _mm_mul_ps(P3, v3.vec);
	P0 = _mm_add_ps(P0,P1);
	P2 = _mm_add_ps(P2,P3);
	P0 = _mm_add_ps(P0,P2);
	return P0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::hermite(const float4 &v1, const float4 &t1, const float4 &v2, const float4 &t2, scalar s)
{
	scalar s2 = s * s;
	scalar s3 = s * s2;

	__m128 P0 = _mm_set_ps1(2.0f * s3 - 3.0f * s2 + 1.0f);
	__m128 T0 = _mm_set_ps1(s3 - 2.0f * s2 + s);
	__m128 P1 = _mm_set_ps1(-2.0f * s3 + 3.0f * s2);
	__m128 T1 = _mm_set_ps1(s3 - s2);

	__m128 vResult = _mm_mul_ps(P0, v1.vec);
	__m128 vTemp = _mm_mul_ps(T0, t1.vec);
	vResult = _mm_add_ps(vResult,vTemp);
	vTemp = _mm_mul_ps(P1, v2.vec);
	vResult = _mm_add_ps(vResult,vTemp);
	vTemp = _mm_mul_ps(T1, t2.vec);
	vResult = _mm_add_ps(vResult,vTemp);
	return vResult;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::lerp(const float4 &v0, const float4 &v1, scalar s)
{
	return v0 + ((v1-v0) * s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::maximize(const float4 &v0, const float4 &v1)
{
    return _mm_max_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::minimize(const float4 &v0, const float4 &v1)
{
    return _mm_min_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::normalize(const float4 &v)
{
    if (float4::equal3_all(v, float4(0,0,0,0))) return v;
    return _mm_div_ps(v.vec.vec,_mm_sqrt_ps(_mm_dp_ps(v.vec.vec, v.vec.vec, 0xFF)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::normalizeapprox(const float4 &v)
{
	if (float4::equal3_all(v, float4(0,0,0,0))) return v;
    return _mm_mul_ps(v.vec.vec,_mm_rsqrt_ps(_mm_dp_ps(v.vec.vec, v.vec.vec, 0xFF)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::normalize3(const float4 &v)
{
	if (float4::equal3_all(v, float4(0, 0, 0, 0))) return v;
	return _mm_div_ps(v.vec.vec, _mm_sqrt_ps(_mm_dp_ps(v.vec.vec, v.vec.vec, 0xF7)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::normalizeapprox3(const float4 &v)
{
	if (float4::equal3_all(v, float4(0, 0, 0, 0))) return v;
	return _mm_mul_ps(v.vec.vec, _mm_rsqrt_ps(_mm_dp_ps(v.vec.vec, v.vec.vec, 0xF7)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::reflect(const float4 &normal, const float4 &incident)
{
	__m128 res = _mm_mul_ps( incident.vec, normal.vec);
	res = _mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0,0,0,0)),
		_mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1,1,1,1)), _mm_shuffle_ps(res, res, _MM_SHUFFLE(2,2,2,2))));
	res = _mm_add_ps(res, res);
	res = _mm_mul_ps(res, normal.vec);
	res = _mm_sub_ps(incident.vec,res);
	return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::perspective_div(const float4 &v)
{
    __m128 d = _mm_set_ps1(1.0f / v.w());
    return _mm_mul_ps(v.vec, d);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less4_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res != 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less4_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal4_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res != 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal4_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater4_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater4_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res == 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal4_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal4_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res == 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal4_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpeq_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal4_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpeq_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp);
	return res == 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::nearequal4(const float4 &v0, const float4 &v1, const float4 &epsilon)
{
	__m128 delta = _mm_sub_ps(v0.vec,v1.vec);
	__m128 temp = _mm_setzero_ps();
	temp = _mm_sub_ps(temp,delta);
	temp = _mm_max_ps(temp,delta);
	temp = _mm_cmple_ps(temp,epsilon.vec);
	return (_mm_movemask_ps(temp)==0xf) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less3_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res != 7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::less3_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal3_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res != 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::lessequal3_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater3_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greater3_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpgt_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal3_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::greaterequal3_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpge_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal3_any(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpeq_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::equal3_all(const float4 &v0, const float4 &v1)
{
	__m128 vTemp = _mm_cmpeq_ps(v0.vec,v1.vec);
	int res = _mm_movemask_ps(vTemp) & 7;
	return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
float4::nearequal3(const float4 &v0, const float4 &v1, const float4 &epsilon)
{
	__m128 delta = _mm_sub_ps(v0.vec,v1.vec);
	__m128 temp = _mm_setzero_ps();
	temp = _mm_sub_ps(temp,delta);
	temp = _mm_max_ps(temp,delta);
	temp = _mm_cmple_ps(temp,epsilon.vec);
	return (_mm_movemask_ps(temp)==0x7) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::less(const float4 &v0, const float4 &v1)
{
    return _mm_min_ps(_mm_cmplt_ps(v0.vec.vec, v1.vec.vec),_plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::greater(const float4 &v0, const float4 &v1)
{
    return _mm_min_ps(_mm_cmpgt_ps(v0.vec.vec, v1.vec.vec),_plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::equal(const float4 &v0, const float4 &v1)
{
    return _mm_min_ps(_mm_cmpeq_ps(v0.vec.vec, v1.vec.vec),_plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_x(__m128 v)
{
    scalar x;
	_mm_store_ss(&x,v);
    return x;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_y(__m128 v)
{
	scalar y;
	__m128 temp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(1,1,1,1));
	_mm_store_ss(&y,temp);
	return y;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_z(__m128 v)
{
	scalar y;
	__m128 temp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(2,2,2,2));
	_mm_store_ss(&y,temp);
	return y;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
float4::unpack_w(__m128 v)
{
	scalar y;
	__m128 temp = _mm_shuffle_ps(v,v,_MM_SHUFFLE(3,3,3,3));
	_mm_store_ss(&y,temp);
	return y;
}
//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat(scalar s)
{
	__m128 temp = _mm_set_ss(s);
	temp = _mm_shuffle_ps( temp, temp, _MM_SHUFFLE(0, 0, 0, 0) );

	return temp;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat(const float4 &v, uint element)
{
    n_assert(element < 4 && element >= 0);
    
	switch(element)
	{
	case 0:
		return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
	case 1:
		return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
	case 2:
		return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
	}
	return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(3, 3, 3, 3));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_x(const float4 &v)
{
    return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_y(const float4 &v)
{
    return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_z(const float4 &v)
{
    return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
float4
float4::splat_w(const float4 &v)
{
    return _mm_shuffle_ps( v.vec, v.vec, _MM_SHUFFLE(3, 3, 3, 3));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::select(const float4& v0, const float4& v1, const uint i0, const uint i1, const uint i2, const uint i3)
{
	//FIXME this should be converted to something similiar as XMVectorSelect
	return float4::permute(v0, v1, i0, i1, i2, i3);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::permute(const float4& v0, const float4& v1, unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3)
{
	static mm128_ivec three = {3,3,3,3};

	NEBULA_ALIGN16 unsigned int elem[4] = { i0, i1, i2, i3 };
	__m128i vControl = _mm_load_si128( reinterpret_cast<const __m128i *>(&elem[0]) );

	__m128i vSelect = _mm_cmpgt_epi32( vControl, (reinterpret_cast<const __m128i *>(&three)[0]));
	vControl = _mm_castps_si128( _mm_and_ps( _mm_castsi128_ps( vControl ), three ) );

	__m128 shuffled1 = _mm_permutevar_ps( v0.vec, vControl );
	__m128 shuffled2 = _mm_permutevar_ps( v1.vec, vControl );

	__m128 masked1 = _mm_andnot_ps( _mm_castsi128_ps( vSelect ), shuffled1 );
	__m128 masked2 = _mm_and_ps( _mm_castsi128_ps( vSelect ), shuffled2 );

	return _mm_or_ps( masked1, masked2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::floor(const float4 &v)
{
	return _mm_floor_ps(v.vec.vec);	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float4
float4::ceiling(const float4 &v)
{
	return _mm_ceil_ps(v.vec.vec);	
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








