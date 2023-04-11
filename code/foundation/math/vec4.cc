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
vec4::load_ubyte4n(const void* ptr)
{
    // need to endian-convert the source...
    uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);
    __m128i vals = _mm_set1_epi32(val);
    vals = _mm_cvtepu8_epi32(vals);
    __m128 fvals = _mm_cvtepi32_ps(vals);
    const __m128 norm = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };
    fvals = _mm_mul_ps(fvals, norm);
    this->vec = fvals;
}

//------------------------------------------------------------------------------
/**
*/
void
vec4::load_byte4n(const void* ptr)
{
    // need to endian-convert the source...
    uint val = System::ByteOrder::Convert<uint>(System::ByteOrder::Host, System::ByteOrder::LittleEndian, *(uint*)ptr);
    __m128i vals = _mm_set1_epi32(val);
    vals = _mm_cvtepu8_epi32(vals);
    __m128 fvals = _mm_cvtepi32_ps(vals);
    const __m128 norm = { 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f };
    fvals = _mm_mul_ps(fvals, norm);
    fvals = _mm_add_ps(fvals, _mm_set_ps1(-0.5f));
    fvals = _mm_mul_ps(fvals, _mm_set_ps1(2.0f));
    this->vec = fvals;
}

} // namespace Math
