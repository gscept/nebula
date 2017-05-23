#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::quaternion

    A quaternion class on top of the VectorMath math functions.

    (C) 2007 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "math/float4.h"

#ifndef WIN32
#define BT_USE_SSE
#endif
#include "vectormath/vmInclude.h"
//------------------------------------------------------------------------------
namespace Math
{
class quaternion;

typedef Vectormath::Aos::Quat vmQuat;

#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
typedef const quaternion  __QuaternionArg;
#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const quaternion& __QuaternionArg;
#endif


class NEBULA3_ALIGN16 quaternion
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
	/// construct from a Quat
	quaternion(const Vectormath::Aos::Quat & rhs);

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

    Vectormath::Aos::Quat vec;	

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
    this->vec = Vectormath::Aos::Quat(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion(float4 const &rhs) :
    vec(rhs.vec.vec)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion(const __m128 & rhs) :
    vec(rhs)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quaternion::quaternion(const Vectormath::Aos::Quat & rhs) :
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
quaternion::operator=(const __m128 & rhs)
{
    this->vec = Vectormath::Aos::Quat(rhs);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator==(const quaternion& rhs) const
{
	return float4(rhs.vec.get128()) == float4(this->vec.get128());
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quaternion::operator!=(const quaternion& rhs) const
{
    return !(this->operator==(rhs));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::load(const scalar* ptr)
{
	float4 temp;
	temp.load(ptr);
    this->vec = Vectormath::Aos::Quat(temp.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::loadu(const scalar* ptr)
{
	float4 temp;
	temp.loadu(ptr);
	this->vec = Vectormath::Aos::Quat(temp.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::store(scalar* ptr) const
{
	float4 temp(vec.get128());
	temp.store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::storeu(scalar* ptr) const
{
	float4 temp(vec.get128());
	temp.storeu(ptr);
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
    this->vec = Vectormath::Aos::Quat(x,y,z,w);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_x(scalar x)
{
    this->vec.setX(x);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_y(scalar y)
{
    this->vec.setY(y);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_z(scalar z)
{
    this->vec.setZ(z);
}

//------------------------------------------------------------------------------
/**
*/
inline void
quaternion::set_w(scalar w)
{
    this->vec.setW(w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::set(float4 const &f4)
{
    this->vec = Vectormath::Aos::Quat(f4.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::x()
{
	return ((mm128_vec&)(this->vec.get128Ref())).f[0];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::x() const
{
    return this->vec.getX();
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::y()
{
	return ((mm128_vec&)(this->vec.get128Ref())).f[1];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::y() const
{
    return this->vec.getY();
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::z()
{
	return ((mm128_vec&)(this->vec.get128Ref())).f[2];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::z() const
{
    return this->vec.getZ();
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
quaternion::w()
{
	return ((mm128_vec&)(this->vec.get128Ref())).f[3];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::w() const
{
    return this->vec.getW();
}

//------------------------------------------------------------------------------
/**
*/
__forceinline __m128
quaternion::m128() const
{
    return this->vec.get128();
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
    return Vectormath::Aos::length(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::lengthsq() const
{
	return Vectormath::floatInVec(_vmathVfDot4( this->vec.get128(), this->vec.get128()), 0 );
	//return Vectormath::Aos::lengthSqr(this->vec);
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
	const NEBULA3_ALIGN16 mm128_vec con = {-1.0f, -1.0f, -1.0f, 1.0f};
	quaternion qq(_mm_mul_ps(q.vec.get128(),con.vec));
	return qq;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
quaternion::dot(const quaternion& q0, const quaternion& q1)
{
	return Vectormath::Aos::dot(q0.vec,q1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::exp(const quaternion& q)
{
    float4 f(q.vec.get128());
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
    return Vectormath::Aos::Quat::identity();
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
		con.vec *= 1.0f / len;
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
quaternion::multiply(const quaternion& q0, const quaternion& q1)
{
    return q1.vec * q0.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::normalize(const quaternion& q)
{
    return Vectormath::Aos::normalize(q.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::rotationaxis(const float4& axis, scalar angle)
{
	return Vectormath::Aos::Quat::rotation(angle, Vectormath::Aos::Vector3(axis.vec));
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
*/
__forceinline quaternion
quaternion::slerp(const quaternion& q1, const quaternion& q2, scalar t)
{
	return Vectormath::Aos::slerp(t,q1.vec,q2.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::squadsetup(const quaternion& q0, const quaternion& q1, const quaternion& q2, const quaternion& q3, quaternion& aOut, quaternion& bOut, quaternion& cOut)
{
	//n_error("not implemented");
    //XMQuaternionSquadSetup(&aOut.vec, &bOut.vec, &cOut.vec, q0.vec, q1.vec, q2.vec, q3.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quaternion
quaternion::squad(const quaternion& q1, const quaternion& a, const quaternion& b, const quaternion& c, scalar t)
{
	return Vectormath::Aos::squad(t,q1.vec,a.vec,b.vec,c.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quaternion::to_axisangle(const quaternion& q, float4& outAxis, scalar& outAngle)
{
	outAxis = q.vec.get128();
	outAxis.set_w(0);
	outAngle = 2.0f * n_acos(q.vec.getW());
    outAxis.set_w(0.0f);
}

} // namespace Math
//------------------------------------------------------------------------------
