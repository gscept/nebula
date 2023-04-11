//------------------------------------------------------------------------------
//  vec4test.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "vec4test.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

static const vec4 E4(0.00001);
static const vec4 E2(0.001);
static const vec4 E1(0.1);

namespace Test
{
__ImplementClass(Test::Vec4Test, 'V4TS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
Vec4Test::Run()
{
    STACK_CHECKPOINT("Test::Vec4Test::Run()");

    // construction
    vec4 v0(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 v1(4.0f, 3.0f, 2.0f, 1.0f);
    vec4 v2(v0);
    vec4 v3(v1);
    VERIFY(v0 == v2);
    VERIFY(v1 == v3);
    VERIFY(v0 != v1);
    VERIFY(v2 != v3);
    VERIFY(v0 == vec4(1.0f, 2.0f, 3.0f, 4.0));

    // assignemt
    v2 = v1;
    VERIFY(v2 == v1);
    v2 = v0;
    VERIFY(v2 == v0);

    // operators
    // float4 operator-() const;
    v0 = -v0;
    VERIFY(v0 == vec4(-1.0f, -2.0f, -3.0f, -4.0f));
    v0 = -v0;
    VERIFY(v0 == v2);
    // void operator+=(const float4& rhs);
    v2 += v3;
    VERIFY(v2 == vec4(5.0f, 5.0f, 5.0f, 5.0f));
    // void operator-=(const float4& rhs);
    v2 -= v3;
    VERIFY(v2 == v0);
    // void operator*=(scalar s);
    v2 *= 2.0f;
    VERIFY(v2 == vec4(2.0f, 4.0f, 6.0f, 8.0f));
    // float4 operator+(const float4& rhs) const;
    v2 = v0 + v1;
    VERIFY(v2 == vec4(5.0f, 5.0f, 5.0f, 5.0f));
    // float4 operator-(const float4& rhs) const;
    v2 = v0 - v1;
    VERIFY(v2 == vec4(-3.0f, -1.0f, 1.0f, 3.0f));
    // float4 operator*(scalar s) const;
    v2 = v0 * 2.0f;
    VERIFY(v2 == vec4(2.0f, 4.0f, 6.0f, 8.0f));

    // setting and getting content
    v2.set(2.0f, 3.0f, 4.0f, 5.0f);
    VERIFY(v2.x == 2.0f);
    VERIFY(v2.y == 3.0f);
    VERIFY(v2.z == 4.0f);
    VERIFY(v2.w == 5.0f);
    VERIFY(v2 == vec4(2.0f, 3.0f, 4.0f, 5.0f));
    v2.x = 1.0f;
    v2.y = 2.0f;
    v2.z = 3.0f;
    v2.w = 4.0f;
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.x = 5.0f;
    v2.y = 6.0f;
    v2.z = 7.0f;
    v2.w = 8.0f;
    VERIFY(v2 == vec4(5.0f, 6.0f, 7.0f, 8.0f));

    // load and store aligned
    NEBULA_ALIGN16 const scalar fAlignedLoad[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    NEBULA_ALIGN16 scalar fAlignedStore[4];
    // check alignment
    n_assert(!((size_t)fAlignedLoad & 0xF));
    n_assert(!((size_t)fAlignedStore & 0xF));
    v2.load(fAlignedLoad);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    // load unaligned must work with aligned data too
    v2.loadu(fAlignedLoad);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.load_float3(fAlignedLoad,4.0f);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.store(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
    // store unaligned must work with aligned data too
    v2.storeu(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
    v2.stream(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
    
    // load and store unaligned
    NEBULA_ALIGN16 const scalar fAlignedLoadBase[5] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
    NEBULA_ALIGN16 scalar fAlignedStoreBase[5];
    const scalar *fUnalignedLoad = fAlignedLoadBase + 1;
    scalar *fUnalignedStore = fAlignedStoreBase + 1;
    // check un-alignment
    n_assert(((size_t)fUnalignedLoad & 0xF));
    n_assert(((size_t)fUnalignedStore & 0xF));
    v2.loadu(fUnalignedLoad);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.storeu(fUnalignedStore);
    VERIFY((fUnalignedStore[0] == 1.0f) && (fUnalignedStore[1] == 2.0f) && (fUnalignedStore[2] == 3.0f) && (fUnalignedStore[3] == 4.0f));

    // 255, 127, 0, 0 -> 1, 0.5, 0, 0
    const uint ubyte4 = 0xFF7F0000;
    v2.load_ubyte4n(&ubyte4);
    VERIFY(nearequal(v2, vec4(0.0f, 0.0f, 0.5f, 1.0f), E1));

    // 255, 127, 0, 0 -> 1, 0, -1, -1
    v2.load_byte4n(&ubyte4);
    VERIFY(nearequal(v2, vec4(-1.0f, -1.0f, 0.0f, 1.0f), E1));


/*
    // load_ubyte4n_signed
    NEBULA_ALIGN16 const uchar ucArrAligned[8] = {0, 0, 128, 255, 0xEE, 0xDD, 0xBB, 0xAA };
    // check alignment
    n_assert(!((unsigned int)ucArrAligned & 0xF));
    const uchar *ucArrUnaligned = ucArrAligned + 1;
    // check un-alignment

    // access ucArrAligned[1]
    n_assert(((unsigned int)ucArrUnaligned & 0xF));
    v2.load_ubyte4n_signed(ucArrUnaligned, 2.0f);
print(v2);
    //VERIFY(float4equal(v2, float4(-1.0f, 0.003922f, 1.0f, 2.0f)));

    // access ucArrAligned[2]
    ++ucArrUnaligned;
    v2.load_ubyte4n_signed(ucArrUnaligned, 2.0f);
print(v2);
    //VERIFY(float4equal(v2, float4(-1.0f, 0.003922f, 1.0f, 2.0f)));

    // access ucArrAligned[3]
    ++ucArrUnaligned;
    v2.load_ubyte4n_signed(ucArrUnaligned, 2.0f);
print(v2);
    //VERIFY(float4equal(v2, float4(-1.0f, 0.003922f, 1.0f, 2.0f)));

    // access ucArrAligned[4]
    ++ucArrUnaligned;
    v2.load_ubyte4n_signed(ucArrUnaligned, 2.0f);
print(v2);
    //VERIFY(float4equal(v2, float4(-1.0f, 0.003922f, 1.0f, 2.0f)));
*/

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_))
    {
        testStackAlignment16<vec4>(this);
    }
#endif

    // length and abs
    v2.set(1.0f, 2.0f, 3.0f, 5.0f);
    VERIFY(Math::fequal(length(v2), 6.244997998f, 0.0001f));
    VERIFY(Math::fequal(length3(v2), 3.741657387f, 0.0001f));
    VERIFY(Math::fequal(lengthsq(v2), 39.0f, 0.0001f));
    v2.set(-1.0f, 2.0f, -3.0f, 4.0f);
    VERIFY(abs(v2) == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    
    // reciprocal
    v0.set(1.0f, 10.0f, 5.0f, 2.0f);
    v0 = reciprocal(v0);
    VERIFY(nearequal(v0, vec4(1.0f, 0.1f, 0.2f, 0.5f), E4));

    // cross3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(0.0f, 0.0f, 1.0f, 0.0f);
    v2 = cross3(v0, v1);
    VERIFY(nearequal(v2, vec4(0.0f, -1.0f, 0.0f, 0.0f), E4));

    // multiply
    v0.set(1.0f, 2.0f, -3.0f, -4.0f);
    v1.set(5.0f, 6.0f, -7.0f, 8.0f);
    v2 = multiply(v0, v1);
    VERIFY(nearequal(v2, vec4(5.0f, 12.0f, 21.0f, -32.0f), E4));

    // dot3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(1.0f, 0.0f, 0.0f, 0.0f);    
    VERIFY(dot3(v0, v1) == 1.0f);
    v1.set(-1.0f, 0.0f, 0.0f, 0.0f);
    VERIFY(dot3(v0, v1) == -1.0f);
    v1.set(0.0f, 1.0f, 0.0f, 0.0f);
    VERIFY(dot3(v0, v1) == 0.0f);
    v0.set(1.0f, 2.0f, 3.0f, 0.0f);
    v1.set(2.0f, -2.0f, -5.0f, 0.0f);
    VERIFY(dot3(v0, v1) == -17.0f);
    v0.set(1.0f, 2.0f, 3.0f, 3.0f);
    v1.set(2.0f, -2.0f, -5.0f, 4.0f);
    VERIFY(dot3(v0, v1) == -17.0f);



    // @todo: test barycentric(), catmullrom(), hermite()

    // lerp
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(2.0f, 3.0f, 4.0f, 5.0f);
    v2 = lerp(v0, v1, 0.5f);
    VERIFY(v2 == vec4(1.5f, 2.5f, 3.5f, 4.5f));
    v2 = lerp(v0, v1, 0.0f);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2 = lerp(v0, v1, 1.0f);
    VERIFY(v2 == vec4(2.0f, 3.0f, 4.0f, 5.0f));
    
    // maximize/minimize
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(4.0f, 3.0f, 2.0f, 1.0f);
    v2 = maximize(v0, v1);
    VERIFY(v2 == vec4(4.0f, 3.0f, 3.0f, 4.0f));
    v2 = minimize(v0, v1);
    VERIFY(v2 == vec4(1.0f, 2.0f, 2.0f, 1.0f));

    // normalize
    v0.set(2.5f, 0.0f, 0.0f, 0.0f);
    v1 = normalize(v0);
    VERIFY(v1 == vec4(1.0f, 0.0f, 0.0f, 0.0f));

    v0.set(4.0f, 2.0f, 3.0f, 2.0f);
    v1 = normalize3(v0);
    VERIFY(nearequal(v1, vec4(0.742781341f, 0.371390671f, 0.557086051f, 2.0f), E4));

    v0.set(4.0f, 2.0f, 3.0f, 2.0f);
    v1 = normalize3approx(v0);
    VERIFY(nearequal(v1, vec4(0.742781341f, 0.371390671f, 0.557086051f, 2.0f), E2));
    
    // transform (point and vector)
    mat4 m = translation(1.0f, 2.0f, 3.0f);
    v0.set(1.0f, 0.0f, 0.0f, 1.0f);
    v1 = m * v0;
    VERIFY(v1 == vec4(2.0f, 2.0f, 3.0f, 1.0f));
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1 = m * v0;
    VERIFY(v0 == vec4(1.0f, 0.0f, 0.0f, 0.0f));
    const mat4 m0(vec4(1.0f, 0.0f, 0.0f, 0.0f),
        vec4(0.0f, 1.0f, 0.0f, 0.0f),
        vec4(0.0f, 0.0f, 1.0f, 0.0f),
        vec4(1.0f, 2.0f, 3.0f, 1.0f));
    const vec4 in(0.0f, 0.0f, 0.0f, 1.0f);
    const vec4 out = m0 * in;
    VERIFY(nearequal(out, vec4(1.0f, 2.0f, 3.0f, 1.0f), E4));

    // component-wise comparison
    const vec4 v0000(0.0f, 0.0f, 0.0f, 0.0f);
    const vec4 v0011(0.0f, 0.0f, 1.0f, 1.0f);
    const vec4 v1111(1.0f, 1.0f, 1.0f, 1.0f);
    const vec4 v1222(1.0f, 2.0f, 2.0f, 2.0f);
    const vec4 v2222(2.0f, 2.0f, 2.0f, 2.0f);
    // 3 components
    VERIFY(!less3_any(v1222, v1111));
    VERIFY( less3_any(v1222, v2222));
    VERIFY(!less3_all(v1111, v1222));
    VERIFY( less3_all(v0000, v1222));
    VERIFY( lessequal3_any(v1111, v1222));
    VERIFY(!lessequal3_any(v2222, v1111));
    VERIFY(!lessequal3_all(v1222, v1111));
    VERIFY( lessequal3_all(v2222, v2222));
    VERIFY( lessequal3_all(v1222, v2222));
    VERIFY(!greater3_any(v1222, v2222));
    VERIFY( greater3_any(v2222, v1222));
    VERIFY( greater3_all(v1222, v0000));
    VERIFY(!greater3_all(v1111, v1222));
    VERIFY( greaterequal3_any(v1222, v1111));
    VERIFY(!greaterequal3_any(v0000, v1222));
    VERIFY(!greaterequal3_all(v0011, v1222));
    VERIFY( greaterequal3_all(v1222, v1111));
    VERIFY( greaterequal3_all(v0000, v0000));
    VERIFY( greaterequal3_all(v1111, v0000));
    VERIFY(!equal3_any(v1111, v0000));
    VERIFY( equal3_any(v1111, v1222));
    VERIFY( equal3_any(v1111, v1111));
    VERIFY( equal3_all(v1111, v1111));
    VERIFY(!equal3_all(v1111, v1222));    
    VERIFY( nearequal3(v0000, vec4( 0.001f, 0.1f, 0.05f, 3.0f), vec4(0.001f, 0.1f, 0.05f, 0.0f)));
    VERIFY( nearequal3(v0000, vec4(-0.001f,-0.1f,-0.05f,-3.0f), vec4(0.001f, 0.1f, 0.05f, 0.0f)));
    VERIFY(!nearequal3(v0000, vec4( 0.002f, 0.1f, 0.05f, 3.0f), vec4(0.001f, 0.1f, 0.05f, 0.0f)));
    VERIFY(!nearequal3(v0000, vec4(-0.001f,-0.2f,-0.05f,-3.0f), vec4(0.001f, 0.1f, 0.05f, 0.0f)));
    // 4 components
    VERIFY(!less_any(v2222, v1222));    
    VERIFY( less_any(v1111, v1222));    
    VERIFY( less_any(v1222, v2222));    
    VERIFY(!less_all(v1111, v1222));
    VERIFY( less_all(v0000, v1222));
    VERIFY( lessequal_any(v1111, v1222));
    VERIFY(!lessequal_any(v1222, v0000));
    VERIFY( lessequal_all(v1111, v1222));
    VERIFY( lessequal_all(v1111, v1111));
    VERIFY(!lessequal_all(v1222, v1111));
    VERIFY(!greater_any(v1111, v1111));
    VERIFY(!greater_any(v0000, v1111));
    VERIFY( greater_any(v1222, v1111));
    VERIFY(!greater_all(v1222, v1111));
    VERIFY( greater_all(v1111, v0000));
    VERIFY( greaterequal_any(v1222, v2222));
    VERIFY(!greaterequal_any(v0000, v1111));
    VERIFY( greaterequal_any(v0000, v0000));
    VERIFY( greaterequal_all(v0000, v0000));
    VERIFY(!greaterequal_all(v0000, v0011));
    VERIFY( greaterequal_all(v1111, v0000));
    VERIFY( equal_any(v0011, v0000));
    VERIFY(!equal_any(v1222, v0000));
    VERIFY(!(v0011 == v0000));
    VERIFY(!(v1222 == v0000));
    VERIFY( (v0000 == v0000));
    VERIFY( nearequal(v0000, vec4( 0.001f, 0.1f, 0.05f, 3.0f), vec4(0.001f, 0.1f, 0.05f, 3.0f)));
    VERIFY( nearequal(v0000, vec4(-0.001f,-0.1f,-0.05f,-3.0f), vec4(0.001f, 0.1f, 0.05f, 3.0f)));
    VERIFY(!nearequal(v0000, vec4( 0.002f, 0.1f, 0.05f, 3.0f), vec4(0.001f, 0.1f, 0.05f, 3.0f)));
    VERIFY(!nearequal(v0000, vec4(-0.001f,-0.2f,-0.05f,-3.0f), vec4(0.001f, 0.1f, 0.05f, 3.0f)));

    // check splat
    v1.set(1.0f, 2.0f, 3.0f, 4.0f);
    v0 = splat(v1, 0);
    VERIFY(v0 == vec4(1.0f, 1.0f, 1.0f, 1.0f));
    v0 = splat_x(v1);
    VERIFY(v0 == vec4(1.0f, 1.0f, 1.0f, 1.0f));
    v0 = splat(v1, 1);
    VERIFY(v0 == vec4(2.0f, 2.0f, 2.0f, 2.0f));
    v0 = splat_y(v1);
    VERIFY(v0 == vec4(2.0f, 2.0f, 2.0f, 2.0f));
    v0 = splat(v1, 2);
    VERIFY(v0 == vec4(3.0f, 3.0f, 3.0f, 3.0f));
    v0 = splat_z(v1);
    VERIFY(v0 == vec4(3.0f, 3.0f, 3.0f, 3.0f));
    v0 = splat(v1, 3);
    VERIFY(v0 == vec4(4.0f, 4.0f, 4.0f, 4.0f));
    v0 = splat_w(v1);
    VERIFY(v0 == vec4(4.0f, 4.0f, 4.0f, 4.0f));

    // check permute
    v0.set(1.0f,2.0f,3.0f,4.0f);
    v1.set(5.0f,6.0f,7.0f,8.0f);

    VERIFY(permute(v0,v1,0,1,5,6) == vec4(1.0f,2.0f,6.0f,7.0f));

    // floor/ceil

    v1.set(1.3, 2.5, 3.8, 5.0);
    v0 = ceil(v1);
    VERIFY(v0 == vec4(2.0f, 3.0f, 4.0f, 5.0f));
    v0 = floor(v1);
    VERIFY(v0 == vec4(1.0f, 2.0f, 3.0f, 5.0f));
}

}