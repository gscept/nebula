//------------------------------------------------------------------------------
//  float4.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "system/byteorder.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
float4
float4::transform(const float4 & v, const matrix44 &m)
{
	return matrix44::transform(v,m);    
}

//------------------------------------------------------------------------------
/**
*/
float4
float4::clamp(__Float4Arg vClamp, __Float4Arg vMin, __Float4Arg vMax)
{
	__m128 temp = _mm_max_ps(vMin.vec,vClamp.vec);
	temp = _mm_min_ps(temp,vMax.vec);
	return float4(temp);
}

//------------------------------------------------------------------------------
/**
*/
scalar
float4::angle(const float4 & v0, const float4 &v1)
{

    __m128 l0 = _mm_mul_ps( v0.vec.vec,v0.vec.vec);
	l0 = _mm_add_ps(_mm_shuffle_ps(l0, l0, _MM_SHUFFLE(0,0,0,0)),
		_mm_add_ps(_mm_shuffle_ps(l0,l0, _MM_SHUFFLE(1,1,1,1)), _mm_shuffle_ps(l0, l0, _MM_SHUFFLE(2,2,2,2))));

    __m128 l1 = _mm_mul_ps( v1.vec.vec,v1.vec.vec);
	l1 = _mm_add_ps(_mm_shuffle_ps(l1, l1, _MM_SHUFFLE(0,0,0,0)),
		_mm_add_ps(_mm_shuffle_ps(l1, l1, _MM_SHUFFLE(1,1,1,1)), _mm_shuffle_ps(l1, l1, _MM_SHUFFLE(2,2,2,2))));

    __m128 l = _mm_shuffle_ps(l0,l1, _MM_SHUFFLE(0,0,0,0));
    l = _mm_rsqrt_ps(l);
    l = _mm_mul_ss(_mm_shuffle_ps(l,l,_MM_SHUFFLE(0,0,0,0)),_mm_shuffle_ps(l,l,_MM_SHUFFLE(1,1,1,1)));


    __m128 dot = _mm_mul_ps( v0.vec, v1.vec);
	dot =_mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(0,0,0,0)),
		_mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(1,1,1,1)),
		_mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(2,2,2,2)), _mm_shuffle_ps(dot, dot, _MM_SHUFFLE(3,3,3,3)))));

    dot = _mm_mul_ss(dot,l);

    dot = _mm_max_ss(dot, _minus1);
    dot = _mm_min_ss(dot, _plus1);

    scalar cangle = float4::unpack_x(dot);
    return n_acos(cangle);
}

//------------------------------------------------------------------------------
/**
*/
float4
float4::select( const float4& v0, const float4& v1, const float4& control )
{
	__m128 v0masked = _mm_andnot_ps(control.vec.vec, v0.vec.vec);
	__m128 v1masked = _mm_and_ps(v1.vec.vec, control.vec.vec);
    return _mm_or_ps(v0masked, v1masked);
}

//------------------------------------------------------------------------------
/**
*/
void
float4::load_ubyte4n_signed(const void* ptr, float w)
{
    // need to endian-convert the source...
    uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);	
    __m128i vals = _mm_set1_epi32(val);
    vals = _mm_cvtepi8_epi32(vals);
    __m128 fvals = _mm_cvtepi32_ps(vals);
	const mm128_vec norm = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };
    fvals = _mm_mul_ps(fvals,norm.vec);
    __m128 two = _mm_set1_ps(2.0f);
    this->vec.vec = _mm_sub_ps(_mm_mul_ps(two, fvals),_plus1);

    this->set_w(w);
}

//------------------------------------------------------------------------------
/**
*/
void
float4::load_byte4n(const void* ptr, float w)
{
	// need to endian-convert the source...
	uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);
	__m128i vals = _mm_set1_epi32(val);
	vals = _mm_cvtepi8_epi32(vals);
	__m128 fvals = _mm_cvtepi32_ps(vals);
	const mm128_vec norm = { 1.0f / 126.0f, 1.0f / 126.0f, 1.0f / 126.0f, 1.0f / 126.0f };
	fvals = _mm_mul_ps(fvals, norm.vec);
	this->vec.vec = fvals;

	this->set_w(w);
}

} // namespace Math
