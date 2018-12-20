#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::quaternion

    A quaternion class using SSE

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
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


class NEBULA_ALIGN16 quaternion
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
    /// construct from __m128
    quaternion(const __m128 & rhs);
	
    /// assignment operator
    void operator=(const quaternion& rhs);
    /// assign __m128
    void operator=(const __m128 & rhs);
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
    ///
    __m128 m128() const;

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
	/// convert to euler angles
	static void to_euler(const quaternion& q, float4& outangles);
private:
    friend class matrix44;

    mm128_vec vec;

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
	this->vec.vec = _mm_setr_ps(x, y, z, w);
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
quaternion::quaternion(const __m128 & rhs)    
{
	this->vec.vec = rhs;
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
quaternion::operator=(const __m128 & rhs)
{
	this->vec.vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator==(const quaternion& rhs) const
{
	return _mm_movemask_ps(_mm_cmpeq_ps(this->vec.vec, rhs.vec.vec)) == 0x0f;	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator!=(const quaternion& rhs) const
{
	return _mm_movemask_ps(_mm_cmpeq_ps(this->vec.vec, rhs.vec.vec)) != 0x0f;
}

//------------------------------------------------------------------------------
/**
	Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
quaternion::load(const scalar* ptr)
{
	this->vec.vec = _mm_load_ps(ptr);
}

//------------------------------------------------------------------------------
/**
	Load 4 floats from unaligned memory.
*/
__forceinline void
quaternion::loadu(const scalar* ptr)
{
	this->vec.vec = _mm_loadu_ps(ptr);
}

//------------------------------------------------------------------------------
/**
	Store to 16-byte-aligned float pointer.
*/
__forceinline void
quaternion::store(scalar* ptr) const
{
	_mm_store_ps(ptr, this->vec.vec);
}

//------------------------------------------------------------------------------
/**
	Store to non-aligned float pointer.
*/
__forceinline void
quaternion::storeu(scalar* ptr) const
{
	_mm_storeu_ps(ptr, this->vec.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::stream(scalar* ptr) const
{
    this->store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::set(scalar x, scalar y, scalar z, scalar w)
{
	this->vec.vec = _mm_setr_ps(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_x(scalar x)
{
	__m128 temp = _mm_set_ss(x);
	this->vec.vec = _mm_move_ss(this->vec.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_y(scalar y)
{
	__m128 temp2 = _mm_load_ps1(&y);
	this->vec.vec = _mm_blend_ps(this->vec.vec, temp2, 2);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_z(scalar z)
{
	__m128 temp2 = _mm_load_ps1(&z);
	this->vec.vec = _mm_blend_ps(this->vec.vec, temp2, 4);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_w(scalar w)
{
	__m128 temp2 = _mm_load_ps1(&w);
	this->vec.vec = _mm_blend_ps(this->vec.vec, temp2, 8);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::set(float4 const &f4)
{
	this->vec.vec = f4.vec.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::x()
{
	return this->vec.f[0];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::x() const
{
	scalar temp;
	_mm_store_ss(&temp, this->vec.vec);
	return temp;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::y()
{
	return this->vec.f[1];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::y() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec, this->vec.vec, _MM_SHUFFLE(1, 1, 1, 1));
	_mm_store_ss(&ret, temp);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::z()
{
	return this->vec.f[2];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::z() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec, this->vec.vec, _MM_SHUFFLE(2, 2, 2, 2));
	_mm_store_ss(&ret, temp);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::w()
{
	return this->vec.f[3];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::w() const
{
	scalar ret;
	__m128 temp = _mm_shuffle_ps(this->vec.vec, this->vec.vec, _MM_SHUFFLE(3, 3, 3, 3));
	_mm_store_ss(&ret, temp);
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline __m128
quaternion::m128() const
{
	return this->vec.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::isidentity() const
{
	const quaternion id(0,0,0,1);
    return id == *this;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::length() const
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(this->vec.vec, this->vec.vec, 0xF1)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::lengthsq() const
{
	return _mm_cvtss_f32(_mm_dp_ps(this->vec.vec, this->vec.vec, 0xF1));
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
	scalar s = f + g;
	if(s != 0.0f)
	{
		quaternion a = quaternion::slerp(q0,q1,s);
		quaternion b = quaternion::slerp(q0,q2,s);
		quaternion res = quaternion::slerp(a,b,g/s);
		return res;
	}
	else
	{
		return q0;
	}
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::conjugate(const quaternion& q)
{
	const NEBULA_ALIGN16 mm128_vec con = {-1.0f, -1.0f, -1.0f, 1.0f};	
	quaternion qq(_mm_mul_ps(q.vec.vec,con.vec));
	return qq;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::dot(const quaternion& q0, const quaternion& q1)
{
	return _mm_cvtss_f32(_mm_dp_ps(q0.vec.vec, q1.vec.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::exp(const quaternion& q)
{
    float4 f(q.vec.vec);
	scalar theta = f.length3();
	scalar costheta = n_cos(theta);
	scalar sintheta = n_sin(theta);
	
	f *= sintheta / theta;
	f.set_w(costheta);

	return quaternion(f.vec.vec);	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::identity()
{
	return quaternion(_mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::inverse(const quaternion& q)
{
	scalar len = q.lengthsq();
	if(len > 0.00001f)
	{
		quaternion con = quaternion::conjugate(q);
		__m128 temp = _mm_set1_ps(1.0f / len);
		con.vec.vec = _mm_mul_ps(con.vec.vec, temp);		
		return con;
	}
	return quaternion(0.0f,0.0f,0.0f,0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::ln(const quaternion& q)
{
	
	quaternion ret;

	scalar a = n_acos(q.w());
	scalar isina = 1.0f / n_sin(a);
	
	scalar aisina = a * isina;
	if (isina > 0)
	{
		ret.set(q.x()*aisina, q.y()*aisina,q.z()*aisina,0.0f);		
	} else {
		ret.set(0.0f,0.0f,0.0f,0.0f);		
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::multiply(const quaternion& q1, const quaternion& q0)
{ 
	//FIXME untested
	__m128 rev = _mm_shuffle_ps(q0.vec.vec, q0.vec.vec, _MM_SHUFFLE(0, 1, 2, 3));
	__m128 lo = _mm_shuffle_ps(q1.vec.vec, q1.vec.vec, _MM_SHUFFLE(0, 1, 0, 1));
	__m128 hi = _mm_shuffle_ps(q1.vec.vec, q1.vec.vec, _MM_SHUFFLE(2, 3, 2, 3));

	__m128 tmp1  = _mm_hsub_ps(_mm_mul_ps(q0.vec.vec, lo), _mm_mul_ps(rev, hi));

	__m128 tmp2 = _mm_hadd_ps(_mm_mul_ps(q0.vec.vec, hi), _mm_mul_ps(rev, lo));

	__m128 tmp3 = _mm_addsub_ps(_mm_shuffle_ps(tmp2, tmp1, _MM_SHUFFLE(3, 2, 1, 0)),
						_mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(2, 3, 0, 1)));

	return _mm_shuffle_ps(tmp3, tmp3, _MM_SHUFFLE(2, 1, 3, 0));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::normalize(const quaternion& q)
{
	return quaternion(_mm_div_ps(q.vec.vec,_mm_sqrt_ps(_mm_dp_ps(q.vec.vec, q.vec.vec, 0xff))));
}

//------------------------------------------------------------------------------
/**
    quaternion from rotation axis + angle. Axis has to be normalized
*/
__forceinline quaternion
quaternion::rotationaxis(const float4& axis, scalar angle)
{        
    n_assert2(n_nearequal(axis.length3(), 1.0f, 0.001f), "axis needs to be normalized");

    float sinangle = n_sin(0.5f * angle);
    float cosangle = n_cos(0.5f * angle);

    // set w component to 1
    __m128 b = _mm_and_ps(axis.vec.vec, _mask_xyz);
    b = _mm_or_ps(b, _id_w);            
    return _mm_mul_ps(b, _mm_set_ps(cosangle, sinangle, sinangle, sinangle));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll)
{

	scalar halfYaw = 0.5f * yaw;
    scalar halfPitch = 0.5f * pitch;
    scalar halfRoll = 0.5f * roll;
    scalar cosYaw = n_cos(halfYaw);
	scalar sinYaw = n_sin(halfYaw);
	scalar cosPitch = n_cos(halfPitch);
	scalar sinPitch = n_sin(halfPitch);
	scalar cosRoll = n_cos(halfRoll);
    scalar sinRoll = n_sin(halfRoll);
	quaternion q(-(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw),
                    -(cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw),
                    -(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw),
                    -(cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw));


    return q;
}

//------------------------------------------------------------------------------
/**
    quaternion slerp
    TODO: rewrite using sse/avx
*/
__forceinline quaternion
quaternion::slerp(const quaternion& q1, const quaternion& q2, scalar t)
{    
    __m128 to;

    float qdot = dot(q1, q2);
    // flip when negative angle
    if (qdot < 0)
    {
        qdot = -qdot;
        to = _mm_mul_ps(q2.vec.vec, _mm_set_ps1(-1.0f));
    }
    else
    {
        to = q2.vec.vec;
    }
        
    // just lerp when angle between is narrow
    if (qdot < 0.95f)
    {
        //dont break acos
        float clamped = n_clamp(qdot, -1.0f, 1.0f);
        float angle = n_acos(clamped);
        
        float sin_angle = n_sin(angle);
        float sin_angle_t = n_sin(angle*t);
        float sin_omega_t = n_sin(angle*(1.0f - t));

        __m128 s0 = _mm_set_ps1(sin_omega_t);
        __m128 s1 = _mm_set_ps1(sin_angle_t);
        __m128 sin_div = _mm_set_ps1(1.0f / sin_angle);
        return  _mm_mul_ps (_mm_add_ps(_mm_mul_ps(q1.vec.vec, s0), _mm_mul_ps(to, s1)), sin_div);
    }
    else
    { 
        
        float scale0 = 1.0f - t;
        float scale1 = t;
        __m128 s0 = _mm_set_ps1(scale0);
        __m128 s1 = _mm_set_ps1(scale1);

        return _mm_add_ps(_mm_mul_ps(q1.vec.vec, s0), _mm_mul_ps(to, s1));
    }    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::squadsetup(const quaternion& q0, const quaternion& q1, const quaternion& q2, const quaternion& q3, quaternion& aOut, quaternion& bOut, quaternion& cOut)
{
	n_error("FIXME: not implemented");

    // not sure what this is useful for or what it exactly does. 
    //XMQuaternionSquadSetup(&aOut.vec, &bOut.vec, &cOut.vec, q0.vec, q1.vec, q2.vec, q3.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::squad(const quaternion& q1, const quaternion& a, const quaternion& b, const quaternion& c, scalar t)
{
	return slerp(slerp(q1, c, t), slerp(a, b, t), 2.0f * t * (1.0f - t));	
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::to_axisangle(const quaternion& q, float4& outAxis, scalar& outAngle)
{
	outAxis = q.vec.vec;
	outAxis.set_w(0);
	outAngle = 2.0f * n_acos(q.w());
    outAxis.set_w(0.0f);
}

} // namespace Math
//------------------------------------------------------------------------------
