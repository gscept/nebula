//------------------------------------------------------------------------------
//  vec4.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/vec4.h"
#include "system/byteorder.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
*/
void
vec4::load_ubyte4n_signed(const void* ptr, float w)
{
    // need to endian-convert the source...
    uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);	
    __m128i vals = _mm_set1_epi32(val);
    vals = _mm_cvtepi8_epi32(vals);
    __m128 fvals = _mm_cvtepi32_ps(vals);
	const __m128 norm = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };
    fvals = _mm_mul_ps(fvals,norm);
    __m128 two = _mm_set1_ps(2.0f);
    this->vec = _mm_sub_ps(_mm_mul_ps(two, fvals), _plus1);

    this->w = w;
}

//------------------------------------------------------------------------------
/**
*/
void
vec4::load_byte4n(const void* ptr, float w)
{
	// need to endian-convert the source...
	uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);
	__m128i vals = _mm_set1_epi32(val);
	vals = _mm_cvtepi8_epi32(vals);
	__m128 fvals = _mm_cvtepi32_ps(vals);
	const __m128 norm = { 1.0f / 126.0f, 1.0f / 126.0f, 1.0f / 126.0f, 1.0f / 126.0f };
	fvals = _mm_mul_ps(fvals, norm);
	this->vec = fvals;

	this->w = w;
}

} // namespace Math
