#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::quat

    A quat class using SSE

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace Math
{
struct quat;

quat slerp(const quat& q1, const quat& q2, scalar t);

quat rotationmatrix(const mat4& m);
void to_euler(const quat& q, vec4& outangles);

struct NEBULA_ALIGN16 quat
{
public:
    /// default constructor, NOTE: does NOT setup components!
    quat();
	/// default copy constructor
	quat(quat const&) = default;
    /// construct from components
    quat(scalar x, scalar y, scalar z, scalar w);
    /// construct from vec4
    quat(const vec4& rhs);
    /// construct from __m128
    quat(const __m128& rhs);
	
    /// assign __m128
    void operator=(const __m128& rhs);
    /// equality operator
    bool operator==(const quat& rhs) const;
    /// inequality operator
    bool operator!=(const quat& rhs) const;

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
    /// set from vec4
    void set(vec4 const &f4);

    friend struct mat4;
	union
	{
		__m128 vec;
		struct
		{
			float x, y, z, w;
		};
	};
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
quat::quat()	
{
	this->vec = _mm_setr_ps(0, 0, 0, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quat::quat(scalar x, scalar y, scalar z, scalar w)
{
	this->vec = _mm_setr_ps(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quat::quat(const vec4& rhs) :
    vec(rhs.vec)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
quat::quat(const __m128& rhs)
{
	this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quat::operator=(const __m128& rhs)
{
	this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quat::operator==(const quat& rhs) const
{
	return _mm_movemask_ps(_mm_cmpeq_ps(this->vec, rhs.vec)) == 0x0f;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
quat::operator!=(const quat& rhs) const
{
	return _mm_movemask_ps(_mm_cmpeq_ps(this->vec, rhs.vec)) != 0x0f;
}

//------------------------------------------------------------------------------
/**
	Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
quat::load(const scalar* ptr)
{
	this->vec = _mm_load_ps(ptr);
}

//------------------------------------------------------------------------------
/**
	Load 4 floats from unaligned memory.
*/
__forceinline void
quat::loadu(const scalar* ptr)
{
	this->vec = _mm_loadu_ps(ptr);
}

//------------------------------------------------------------------------------
/**
	Store to 16-byte-aligned float pointer.
*/
__forceinline void
quat::store(scalar* ptr) const
{
	_mm_store_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
	Store to non-aligned float pointer.
*/
__forceinline void
quat::storeu(scalar* ptr) const
{
	_mm_storeu_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quat::stream(scalar* ptr) const
{
	this->store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quat::set(scalar x, scalar y, scalar z, scalar w)
{
	this->vec = _mm_setr_ps(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
quat::set(const vec4& f4)
{
	this->vec = f4.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
isidentity(const quat& q)
{
	const quat id(0, 0, 0, 1);
	return id == q;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
length(const quat& q)
{
	return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(q.vec, q.vec, 0xF1)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq(const quat& q)
{
	return _mm_cvtss_f32(_mm_dp_ps(q.vec, q.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
undenormalize(const quat& q)
{
	// nothing to do on the xbox, since denormal numbers are not supported by the vmx unit,
	// it is being set to zero anyway
	quat ret;
#if __WIN32__
	ret.x = n_undenormalize(q.x);
	ret.y = n_undenormalize(q.y);
	ret.z = n_undenormalize(q.z);
	ret.w = n_undenormalize(q.w);
#endif
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
barycentric(const quat& q0, const quat& q1, const quat& q2, scalar f, scalar g)
{
	scalar s = f + g;
	if (s != 0.0f)
	{
		quat a = slerp(q0, q1, s);
		quat b = slerp(q0, q2, s);
		quat res = slerp(a, b, g / s);
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
__forceinline quat
conjugate(const quat& q)
{
	const  __m128 con = { -1.0f, -1.0f, -1.0f, 1.0f };
	quat qq(_mm_mul_ps(q.vec, con));
	return qq;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const quat& q0, const quat& q1)
{
	return _mm_cvtss_f32(_mm_dp_ps(q0.vec, q1.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
exp(const quat& q)
{
	vec4 f(q.vec);
	scalar theta = length3(f);
	scalar costheta = n_cos(theta);
	scalar sintheta = n_sin(theta);

	f *= sintheta / theta;
	f.w = costheta;

	return quat(f.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
identity()
{
	return quat(_mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
inverse(const quat& q)
{
	scalar len = lengthsq(q);
	if (len > 0.00001f)
	{
		quat con = conjugate(q);
		__m128 temp = _mm_set1_ps(1.0f / len);
		con.vec = _mm_mul_ps(con.vec, temp);
		return con;
	}
	return quat(0.0f, 0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
ln(const quat& q)
{
	quat ret;

	scalar a = n_acos(q.w);
	scalar isina = 1.0f / n_sin(a);

	scalar aisina = a * isina;
	if (isina > 0)
	{
		vec3 mul(aisina, aisina, aisina);
		ret = vec4(vec3(q.vec) * mul, 0.0f);
	}
	else
	{
		ret.set(0.0f, 0.0f, 0.0f, 0.0f);
	}
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
operator*(const quat& q1, const quat& q0)
{
	//FIXME untested
	__m128 rev = _mm_shuffle_ps(q0.vec, q0.vec, _MM_SHUFFLE(0, 1, 2, 3));
	__m128 lo = _mm_shuffle_ps(q1.vec, q1.vec, _MM_SHUFFLE(0, 1, 0, 1));
	__m128 hi = _mm_shuffle_ps(q1.vec, q1.vec, _MM_SHUFFLE(2, 3, 2, 3));

	__m128 tmp1 = _mm_hsub_ps(_mm_mul_ps(q0.vec, lo), _mm_mul_ps(rev, hi));

	__m128 tmp2 = _mm_hadd_ps(_mm_mul_ps(q0.vec, hi), _mm_mul_ps(rev, lo));

	__m128 tmp3 = _mm_addsub_ps(_mm_shuffle_ps(tmp2, tmp1, _MM_SHUFFLE(3, 2, 1, 0)),
		_mm_shuffle_ps(tmp1, tmp2, _MM_SHUFFLE(2, 3, 0, 1)));

	return _mm_shuffle_ps(tmp3, tmp3, _MM_SHUFFLE(2, 1, 3, 0));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
normalize(const quat& q)
{
	return quat(_mm_div_ps(q.vec, _mm_sqrt_ps(_mm_dp_ps(q.vec, q.vec, 0xff))));
}

//------------------------------------------------------------------------------
/**
	quat from rotation axis + angle. Axis has to be normalized
*/
__forceinline quat
rotationquataxis(const vec3& axis, scalar angle)
{
	n_assert2(n_nearequal(length(axis), 1.0f, 0.001f), "axis needs to be normalized");

	float sinangle = n_sin(0.5f * angle);
	float cosangle = n_cos(0.5f * angle);

	// set w component to 1
	__m128 b = _mm_and_ps(axis.vec, _mask_xyz);
	b = _mm_or_ps(b, _id_w);
	return _mm_mul_ps(b, _mm_set_ps(cosangle, sinangle, sinangle, sinangle));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
rotationquatyawpitchroll(scalar yaw, scalar pitch, scalar roll)
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
	quat q(-(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw),
		-(cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw),
		-(sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw),
		-(cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw));


	return q;
}

//------------------------------------------------------------------------------
/**
	quat slerp
	TODO: rewrite using sse/avx
*/
__forceinline quat
slerp(const quat& q1, const quat& q2, scalar t)
{
	__m128 to;

	float qdot = dot(q1, q2);
	// flip when negative angle
	if (qdot < 0)
	{
		qdot = -qdot;
		to = _mm_mul_ps(q2.vec, _mm_set_ps1(-1.0f));
	}
	else
	{
		to = q2.vec;
	}

	// just lerp when angle between is narrow
	if (qdot < 0.95f)
	{
		//dont break acos
		float clamped = n_clamp(qdot, -1.0f, 1.0f);
		float angle = n_acos(clamped);

		float sin_angle = n_sin(angle);
		float sin_angle_t = n_sin(angle * t);
		float sin_omega_t = n_sin(angle * (1.0f - t));

		__m128 s0 = _mm_set_ps1(sin_omega_t);
		__m128 s1 = _mm_set_ps1(sin_angle_t);
		__m128 sin_div = _mm_set_ps1(1.0f / sin_angle);
		return  _mm_mul_ps(_mm_add_ps(_mm_mul_ps(q1.vec, s0), _mm_mul_ps(to, s1)), sin_div);
	}
	else
	{

		float scale0 = 1.0f - t;
		float scale1 = t;
		__m128 s0 = _mm_set_ps1(scale0);
		__m128 s1 = _mm_set_ps1(scale1);

		return _mm_add_ps(_mm_mul_ps(q1.vec, s0), _mm_mul_ps(to, s1));
	}
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
squadsetup(const quat& q0, const quat& q1, const quat& q2, const quat& q3, quat& aOut, quat& bOut, quat& cOut)
{
	n_error("FIXME: not implemented");

	// not sure what this is useful for or what it exactly does. 
	//XMquatSquadSetup(&aOut.vec, &bOut.vec, &cOut.vec, q0.vec, q1.vec, q2.vec, q3.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline quat
squad(const quat& q1, const quat& a, const quat& b, const quat& c, scalar t)
{
	return slerp(slerp(q1, c, t), slerp(a, b, t), 2.0f * t * (1.0f - t));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
to_axisangle(const quat& q, vec4& outAxis, scalar& outAngle)
{
	outAxis = q.vec;
	outAxis.w = 0;
	outAngle = 2.0f * n_acos(q.w);
	outAxis.w = 0.0f;
}

} // namespace Math
//------------------------------------------------------------------------------
