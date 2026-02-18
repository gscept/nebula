#pragma once
//------------------------------------------------------------------------------
/**
    @struct Math::mat4

    A 4x4 single point precision float matrix.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "math/vec4.h"
#include "math/quat.h"
#include "math/point.h"
#include "math/vector.h"
#include "math/sse.h"

//------------------------------------------------------------------------------

#define mm_ror_ps(vec,i)    \
    (((i)%4) ? (_mm_shuffle_ps(vec,vec, _MM_SHUFFLE((unsigned char)(i+3)%4,(unsigned char)(i+2)%4,(unsigned char)(i+1)%4,(unsigned char)(i+0)%4))) : (vec))

namespace Math
{

struct mat4;
struct quat;

static const __m128i maskX = _mm_setr_epi32( -1,0,0,0 );
static const __m128i maskY = _mm_setr_epi32( 0,-1,0,0 );
static const __m128i maskZ = _mm_setr_epi32( 0,0,-1,0 );
static const __m128i maskW = _mm_setr_epi32( 0,0,0,-1 );

mat4 rotationquat(const quat& q);
mat4 reflect(const vec4& p);
void decompose(const mat4& mat, vec3& outScale, quat& outRotation, vec3& outTranslation);
mat4 affine(const vec3& scale, const vec3& rotationCenter, const quat& rotation, const vec3& translation);
mat4 affine(const vec3& scale, const quat& rotation, const vec3& translation);
mat4 affine(const vec3& scale, const vec3& rotation, const vec3& translation);
mat4 affinetransformation(scalar scale, const vec3& rotationCenter, const quat& rotation, const vec3& translation);
mat4 transformation(const vec3& scalingCenter, const quat& scalingRotation, const vec3& scale, const vec3& rotationCenter, const quat& rotation, const vec3& trans);
bool ispointinside(const vec4& p, const mat4& m);
mat4 skewsymmetric(const vec3& v);
mat4 fromeuler(const vec3& v);
vec3 aseuler(const mat4& m);

struct NEBULA_ALIGN16 mat4
{
public:
    /// default constructor. returns identity matrix
    mat4();
    /// copy constructor
    mat4(const mat4& rhs) = default;
    /// construct from components
    mat4(const vec4& row0, const vec4& row1, const vec4& row2, const vec4& row3);
    /// construct from individual values
    mat4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33);

    /// equality operator
    bool operator==(const mat4& rhs) const;
    /// inequality operator
    bool operator!=(const mat4& rhs) const;
    /// row element accessor
    vec4& operator[](size_t const i);
    /// readonly row element accessor
    vec4 const& operator[](size_t const i) const;

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write 3 columns to 16-byte aligned memory through the write cache
    void store3(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// stream content to 16-byte-aligned memory circumventing the write-cache
    void stream(scalar* ptr) const;

    /// set content from row vectors
    void set(const vec4& r0, const vec4& r1, const vec4& r2, const vec4& r3);
    /// set content from individual values
    void set(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23,
        float m30, float m31, float m32, float m33);

    /// extracts scale components to target vector
    void get_scale(vec4& scale) const;
    /// add a translation to pos_component
    void translate(const vec3& t);
    /// add a translation to pos_component
    void translate(const float x, const float y, const float z);
    ///
    vec4 get_x() const;
    ///
    vec4 get_y() const;
    ///
    vec4 get_z() const;
    ///
    vec4 get_w() const;
    /// scale matrix
    void scale(const vec3& v);
    /// scale matrix
    void scale(const float x, const float y, const float z);


    /// we use aliasing to represent the matrix in may different ways
    union
    {
        struct /// as a cube
        {
            vec4 x_axis;
            vec4 y_axis;
            vec4 z_axis;
            vec4 position;
        };

        /// float accessors
        struct /// individual values
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4]; /// as a two-dimensional array

        /// SIMD accessors
        vec4 r[4]; /// array accessible rows 
        struct /// as numbered rows
        {
            vec4 row0;
            vec4 row1;
            vec4 row2;
            vec4 row3;
        };
    };

    static const mat4 identity;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
mat4::mat4() : row0(_id_x), row1(_id_y), row2(_id_z), row3(_id_w)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
mat4::mat4(const vec4& row0, const vec4& row1, const vec4& row2, const vec4& row3)
{
    r[0] = row0.vec;
    r[1] = row1.vec;
    r[2] = row2.vec;
    r[3] = row3.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline 
mat4::mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    this->r[0] = vec4(m00, m01, m02, m03);
    this->r[1] = vec4(m10, m11, m12, m13);
    this->r[2] = vec4(m20, m21, m22, m23);
    this->r[3] = vec4(m30, m31, m32, m33);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
mat4::operator==(const mat4& rhs) const
{
    return vec4(r[0]) == vec4(rhs.r[0]) &&
        vec4(r[1]) == vec4(rhs.r[1]) &&
        vec4(r[2]) == vec4(rhs.r[2]) &&
        vec4(r[3]) == vec4(rhs.r[3]);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
mat4::operator!=(const mat4& rhs) const
{
    return !(*this == rhs);
}

//--------------------------------------------------------------------------
/**
*/
__forceinline vec4 const&
mat4::operator[](size_t const i) const
{
#if NEBULA_BOUNDS_CHECK
    n_assert(i < 4);
#endif
    return this->r[i];
}

//--------------------------------------------------------------------------
/**
*/
__forceinline vec4&
mat4::operator[](size_t const i)
{
#if NEBULA_BOUNDS_CHECK
    n_assert(i < 4);
#endif
    return this->r[i];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::load(const scalar* ptr)
{
    r[0] = _mm_load_ps(ptr);
    r[1] = _mm_load_ps(ptr + 4);
    r[2] = _mm_load_ps(ptr + 8);
    r[3] = _mm_load_ps(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::loadu(const scalar* ptr)
{
    r[0] = _mm_loadu_ps(ptr);
    r[1] = _mm_loadu_ps(ptr + 4);
    r[2] = _mm_loadu_ps(ptr + 8);
    r[3] = _mm_loadu_ps(ptr + 12);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::store(scalar* ptr) const
{
    _mm_store_ps(ptr, r[0].vec);
    _mm_store_ps((ptr + 4), r[1].vec);
    _mm_store_ps((ptr + 8), r[2].vec);
    _mm_store_ps((ptr + 12), r[3].vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::store3(scalar* ptr) const
{
    _mm_store_ps(ptr, r[0].vec);
    _mm_store_ps((ptr + 4), r[1].vec);
    _mm_store_ps((ptr + 8), r[2].vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::storeu(scalar* ptr) const
{
    _mm_storeu_ps(ptr, r[0].vec);
    _mm_storeu_ps((ptr + 4), r[1].vec);
    _mm_storeu_ps((ptr + 8), r[2].vec);
    _mm_storeu_ps((ptr + 12), r[3].vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::stream(scalar* ptr) const
{
    this->storeu(ptr);
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void 
mat4::set(const vec4& r0, const vec4& r1, const vec4& r2, const vec4& r3)
{
    this->r[0] = r0;
    this->r[1] = r1;
    this->r[2] = r2;
    this->r[3] = r3;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void 
mat4::set(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
    this->r[0] = vec4(m00, m01, m02, m03);
    this->r[1] = vec4(m10, m11, m12, m13);
    this->r[2] = vec4(m20, m21, m22, m23);
    this->r[3] = vec4(m30, m31, m32, m33);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::get_scale(vec4& v) const
{
    vec4 xaxis = r[0];
    vec4 yaxis = r[1];
    vec4 zaxis = r[2];
    scalar xScale = length3(xaxis);
    scalar yScale = length3(yaxis);
    scalar zScale = length3(zaxis);

    v = vec4(xScale, yScale, zScale, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
mat4::get_x() const
{
    return this->r[0];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
mat4::get_y() const
{
    return this->r[1];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
mat4::get_z() const
{
    return this->r[2];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
mat4::get_w() const
{
    return this->r[3];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::translate(const vec3& t)
{
    this->r[3] = _mm_add_ps(r[3].vec, t.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
mat4::scale(const vec3& s)
{
    // need to make sure that last column isn't erased
    vec4 scl = vec4(s, 1.0f);

    r[0] = r[0] * scl;
    r[1] = r[1] * scl;
    r[2] = r[2] * scl;
    r[3] = r[3] * scl;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
isidentity(const mat4& m)
{
    return m.r[0] == _id_x &&
        m.r[1] == _id_y &&
        m.r[2] == _id_z &&
        m.r[3] == _id_w;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
determinant(const mat4& m)
{
    __m128 Va,Vb,Vc;
    __m128 r1,r2,r3,tt,tt2;
    __m128 sum,Det;

    __m128 _L1 = m.r[0].vec;
    __m128 _L2 = m.r[1].vec;
    __m128 _L3 = m.r[2].vec;
    __m128 _L4 = m.r[3].vec;
    // Calculating the minterms for the first line.

    // _mm_ror_ps is just a macro using _mm_shuffle_ps().
    tt = _L4; tt2 = mm_ror_ps(_L3,1);
    Vc = _mm_mul_ps(tt2, mm_ror_ps(tt,0));                  // V3' dot V4
    Va = _mm_mul_ps(tt2, mm_ror_ps(tt,2));                  // V3' dot V4"
    Vb = _mm_mul_ps(tt2, mm_ror_ps(tt,3));                  // V3' dot V4^

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));      // V3" dot V4^ - V3^ dot V4"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));      // V3^ dot V4' - V3' dot V4^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));      // V3' dot V4" - V3" dot V4'

    tt = _L2;
    Va = mm_ror_ps(tt,1);       sum = _mm_mul_ps(Va,r1);
    Vb = mm_ror_ps(tt,2);       sum = _mm_add_ps(sum,_mm_mul_ps(Vb,r2));
    Vc = mm_ror_ps(tt,3);       sum = _mm_add_ps(sum,_mm_mul_ps(Vc,r3));

    // Calculating the determinant.
    Det = _mm_mul_ps(sum,_L1);
    Det = _mm_add_ps(Det,_mm_movehl_ps(Det,Det));

    // Calculating the minterms of the second line (using previous results).
    tt = mm_ror_ps(_L1,1);      sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));

    // Testing the determinant.
    Det = _mm_sub_ss(Det,_mm_shuffle_ps(Det,Det,1));
    return vec4(Det).x;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
inverse(const mat4& m)
{
    __m128 Va,Vb,Vc;
    __m128 r1,r2,r3,tt,tt2;
    __m128 sum,Det,RDet;
    __m128 trns0,trns1,trns2,trns3;

    const __m128i pnpn = _mm_setr_epi32(0x00000000, static_cast<int>(0x80000000), 0x00000000, static_cast<int>(0x80000000));
    const __m128i npnp = _mm_setr_epi32(static_cast<int>(0x80000000), 0x00000000, static_cast<int>(0x80000000), 0x00000000);
    const __m128 zeroone = _mm_setr_ps(1.0f, 0.0f, 0.0f, 1.0f);

    __m128 _L1 = m.r[0].vec;
    __m128 _L2 = m.r[1].vec;
    __m128 _L3 = m.r[2].vec;
    __m128 _L4 = m.r[3].vec;
    // Calculating the minterms for the first line.

    // _mm_ror_ps is just a macro using _mm_shuffle_ps().
    tt = _L4; tt2 = mm_ror_ps(_L3,1);
    Vc = _mm_mul_ps(tt2, mm_ror_ps(tt,0));                  // V3'dot V4
    Va = _mm_mul_ps(tt2, mm_ror_ps(tt,2));                  // V3'dot V4"
    Vb = _mm_mul_ps(tt2, mm_ror_ps(tt,3));                  // V3' dot V4^

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));      // V3" dot V4^ - V3^ dot V4"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));      // V3^ dot V4' - V3' dot V4^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));      // V3' dot V4" - V3" dot V4'

    tt = _L2;
    Va = mm_ror_ps(tt,1);       sum = _mm_mul_ps(Va,r1);
    Vb = mm_ror_ps(tt,2);       sum = _mm_add_ps(sum,_mm_mul_ps(Vb,r2));
    Vc = mm_ror_ps(tt,3);       sum = _mm_add_ps(sum,_mm_mul_ps(Vc,r3));

    // Calculating the determinant.
    Det = _mm_mul_ps(sum,_L1);
    Det = _mm_add_ps(Det,_mm_movehl_ps(Det,Det));


    __m128 mtL1 = _mm_xor_ps(sum, _mm_castsi128_ps(pnpn));

    // Calculating the minterms of the second line (using previous results).
    tt = mm_ror_ps(_L1,1);      sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL2 = _mm_xor_ps(sum, _mm_castsi128_ps(npnp));

    // Testing the determinant.
    Det = _mm_sub_ss(Det,_mm_shuffle_ps(Det,Det,1));

    // Calculating the minterms of the third line.
    tt = mm_ror_ps(_L1,1);
    Va = _mm_mul_ps(tt,Vb);                                 // V1' dot V2"
    Vb = _mm_mul_ps(tt,Vc);                                 // V1' dot V2^
    Vc = _mm_mul_ps(tt,_L2);                                // V1' dot V2

    r1 = _mm_sub_ps(mm_ror_ps(Va,1), mm_ror_ps(Vc,2));      // V1" dot V2^ - V1^ dot V2"
    r2 = _mm_sub_ps(mm_ror_ps(Vb,2), mm_ror_ps(Vb,0));      // V1^ dot V2' - V1' dot V2^
    r3 = _mm_sub_ps(mm_ror_ps(Va,0), mm_ror_ps(Vc,1));      // V1' dot V2" - V1" dot V2'

    tt = mm_ror_ps(_L4,1);      sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL3 = _mm_xor_ps(sum, _mm_castsi128_ps(pnpn));

    // Dividing is FASTER than rcp_nr! (Because rcp_nr causes many register-memory RWs).
    RDet = _mm_div_ss(zeroone, Det); // TODO: just 1.0f?
    RDet = _mm_shuffle_ps(RDet,RDet,0x00);

    // Devide the first 12 minterms with the determinant.
    mtL1 = _mm_mul_ps(mtL1, RDet);
    mtL2 = _mm_mul_ps(mtL2, RDet);
    mtL3 = _mm_mul_ps(mtL3, RDet);

    // Calculate the minterms of the forth line and devide by the determinant.
    tt = mm_ror_ps(_L3,1);      sum = _mm_mul_ps(tt,r1);
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r2));
    tt = mm_ror_ps(tt,1);       sum = _mm_add_ps(sum,_mm_mul_ps(tt,r3));
    __m128 mtL4 = _mm_xor_ps(sum, _mm_castsi128_ps(npnp));
    mtL4 = _mm_mul_ps(mtL4, RDet);

    // Now we just have to transpose the minterms matrix.
    trns0 = _mm_unpacklo_ps(mtL1,mtL2);
    trns1 = _mm_unpacklo_ps(mtL3,mtL4);
    trns2 = _mm_unpackhi_ps(mtL1,mtL2);
    trns3 = _mm_unpackhi_ps(mtL3,mtL4);
    _L1 = _mm_movelh_ps(trns0,trns1);
    _L2 = _mm_movehl_ps(trns1,trns0);
    _L3 = _mm_movelh_ps(trns2,trns3);
    _L4 = _mm_movehl_ps(trns3,trns2);

    return mat4(vec4(_L1), vec4(_L2), vec4(_L3), vec4(_L4));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
lookatlh(const point& eye, const point& at, const vector& up)
{
#if NEBULA_DEBUG
    n_assert(length(up) > 0);
#endif
    // hmm the XM lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    const vector zaxis = normalize(at - eye);
    vector normUp = normalize(up);
    if (Math::abs(dot(zaxis, normUp)) > 0.9999999f)
    {
        // need to choose a different up vector because up and lookat point
        // into same or opposite direction
        // just rotate y->x, x->z and z->y
        normUp = permute(normUp, normUp, 1, 2, 0);
    }
    const vector xaxis = normalize(cross(normUp, zaxis));
    const vector yaxis = normalize(cross(zaxis, xaxis));
    return mat4(xaxis, yaxis, zaxis, eye);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
lookatrh(const point& eye, const point& at, const vector& up)
{
#if NEBULA_DEBUG
    n_assert(length(up) > 0);
#endif
    // hmm the XM lookat functions are kinda pointless, because they
    // return a VIEW matrix, which is already inverse (so one would
    // need to reverse again!)
    const vector zaxis = normalize(eye - at);
    vector normUp = normalize(up);
    if (Math::abs(dot(zaxis, normUp)) > 0.9999999f)
    {
        // need to choose a different up vector because up and lookat point
        // into same or opposite direction
        // just rotate y->x, x->z and z->y
        normUp = permute(normUp, normUp, 1, 2, 0);
    }
    const vector xaxis = normalize(cross(normUp, zaxis));
    const vector yaxis = normalize(cross(zaxis, xaxis));
    return mat4(xaxis, yaxis, zaxis, eye);
}

#ifdef N_USE_AVX
// dual linear combination using AVX instructions on YMM regs
static inline __m256 twolincomb_AVX_8(__m256 A01, const mat4 &B)
{
    __m256 result;
    result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps(&B.r[0].vec));
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps(&B.r[1].vec)));
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps(&B.r[2].vec)));
    result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps(&B.r[3].vec)));
    return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
operator*(const mat4& m0, const mat4& m1)
{
    mat4 out;

    _mm256_zeroupper();
    __m256 A01 = _mm256_loadu_ps(&m1.m[0][0]);
    __m256 A23 = _mm256_loadu_ps(&m1.m[2][0]);

    __m256 out01x = twolincomb_AVX_8(A01, m0);
    __m256 out23x = twolincomb_AVX_8(A23, m0);

    _mm256_storeu_ps(&out.m[0][0], out01x);
    _mm256_storeu_ps(&out.m[2][0], out23x);
    return out;
}

__forceinline mat4 operator%(const mat4& m1, const mat4& m0) = delete;

#else
//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
operator*(const mat4& m0, const mat4& m1)
{
    mat4 ret;

    vec4 mw = m1.r[0];

    // Splat the all components of the first row
    vec4 mx = splat_x(mw);
    vec4 my = splat_y(mw);
    vec4 mz = splat_z(mw);
    mw = splat_w(mw);

    vec4 m1x = m0.r[0];
    vec4 m1y = m0.r[1];
    vec4 m1z = m0.r[2];
    vec4 m1w = m0.r[3];

    //multiply first row
    mx = multiply(mx, m1x);
    my = multiply(my, m1y);
    mz = multiply(mz, m1z);
    mw = multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.r[0] = mx + mz;

    // rinse and repeat
    mw = m1.row1;

    mx = splat_x(mw);
    my = splat_y(mw);
    mz = splat_z(mw);
    mw = splat_w(mw);

    mx = multiply(mx, m1x);
    my = multiply(my, m1y);
    mz = multiply(mz, m1z);
    mw = multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.r[1] = mx + mz;

    mw = m1.row2;

    mx = splat_x(mw);
    my = splat_y(mw);
    mz = splat_z(mw);
    mw = splat_w(mw);

    mx = multiply(mx, m0.r[0]);
    my = multiply(my, m0.r[1]);
    mz = multiply(mz, m0.r[2]);
    mw = multiply(mw, m0.r[3]);

    mx = mx + my;
    mz = mz + mw;
    ret.r[2] = mx + mz;

    mw = m1.row3;

    mx = splat_x(mw);
    my = splat_y(mw);
    mz = splat_z(mw);
    mw = splat_w(mw);

    mx = multiply(mx, m1x);
    my = multiply(my, m1y);
    mz = multiply(mz, m1z);
    mw = multiply(mw, m1w);

    mx = mx + my;
    mz = mz + mw;
    ret.r[3] = mx + mz;

    return ret;
}
#endif

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator*(const mat4& m, const vec4& v)
{
    __m128 x = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 y = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 w = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(3, 3, 3, 3));

    return 
        fmadd(x, m.r[0].vec, 
        fmadd(y, m.r[1].vec,
        fmadd(z, m.r[2].vec, 
        _mm_mul_ps(w, m.r[3].vec))));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator*(const mat4& m, const vec3& v)
{
    __m128 x = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
    x = _mm_and_ps(x, _mask_xyz);
    __m128 y = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
    y = _mm_and_ps(y, _mask_xyz);
    __m128 z = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
    z = _mm_and_ps(z, _mask_xyz);

    return 
        fmadd(x, m.r[0].vec, 
        fmadd(y, m.r[1].vec,
        _mm_mul_ps(z, m.r[2].vec)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator*(const mat4& m, const point& p)
{
    __m128 x = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 y = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 w = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(3, 3, 3, 3));

    return 
        fmadd(x, m.r[0].vec, 
        fmadd(y, m.r[1].vec,
        fmadd(z, m.r[2].vec, 
        _mm_mul_ps(w, m.r[3].vec))));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator*(const mat4& m, const vector& v)
{
    __m128 x = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
    x = _mm_and_ps(x, _mask_xyz);
    __m128 y = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
    y = _mm_and_ps(y, _mask_xyz);
    __m128 z = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
    z = _mm_and_ps(z, _mask_xyz);

    return
        fmadd(x, m.r[0].vec,
        fmadd(y, m.r[1].vec,
              _mm_mul_ps(z, m.r[2].vec)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
ortholh(scalar w, scalar h, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar dist = 1.0f / (zf - zn);
    m.r[0] = vec4(2.0f / w, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f / h, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, 0.0f);
    m.r[3] = vec4(0.0f, 0.0, -dist * zn, 1.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
orthorh(scalar w, scalar h, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar dist = 1.0f / (zn - zf);
    m.r[0] = vec4(2.0f / w, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f / h, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, 0.0f);
    m.r[3] = vec4(0.0f, 0.0, dist * zn, 1.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
orthooffcenterlh(scalar l, scalar r, scalar t, scalar b, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    m.r[0] = vec4(2.0f / (r-l), 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f / (t-b), 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, 1.0f / (zf-zn), 0.0f);
    m.r[3] = vec4((l + r) / (l - r), (t + b) / (b - t), zn / (zn - zf), 1.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
orthooffcenterrh(scalar l, scalar r, scalar t, scalar b, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    m.r[0] = vec4(2.0f / (r-l), 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f / (t-b), 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, 1 / (zn-zf), 0.0f);
    m.r[3] = vec4((l + r) / (l - r), (t + b) / (b - t), zn / (zn-zf), 1.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
perspfovlh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar halfFov = 0.5f * fovy;
    scalar sinfov = Math::sin(halfFov);
    scalar cosfov = Math::cos(halfFov);

    scalar height = cosfov / sinfov;
    scalar width = height / aspect;

    scalar dist = zf / (zf - zn);
    m.r[0] = vec4(width, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, height, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, 1.0f);
    m.r[3] = vec4(0.0f, 0.0f, -dist * zn, 0.0f);

    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
perspfovrh(scalar fovy, scalar aspect, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar halfFov = 0.5f * fovy;
    scalar sinfov = Math::sin(halfFov);
    scalar cosfov = Math::cos(halfFov);

    scalar height = cosfov / sinfov;
    scalar width = height / aspect;

    scalar dist = zf / (zn - zf);

    m.r[0] = vec4(width, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, height, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, -1.0f);
    m.r[3] = vec4(0.0f, 0.0f, dist * zn, 0.0f);

    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
persplh(scalar w, scalar h, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar dist = zf / (zf - zn);   

    m.r[0] = vec4(2.0f * zn / w, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f * zn / h, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, 1.0f);
    m.r[3] = vec4(0.0f, 0.0, -dist * zn, 0.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
persprh(scalar w, scalar h, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar dist = zf / (zn - zf);   

    m.r[0] = vec4(2.0f * zn / w, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f * zn / h, 0.0f, 0.0f);
    m.r[2] = vec4(0.0f, 0.0f, dist, -1.0f);
    m.r[3] = vec4(0.0f, 0.0, dist * zn, 0.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
perspoffcenterlh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar divwidth = 1.0f / (r - l);
    scalar divheight = 1.0f / (t - b);
    scalar dist = zf / (zf - zn);

    m.r[0] = vec4(2.0f * zn * divwidth, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f * zn * divheight, 0.0f, 0.0f);
    m.r[2] = vec4(-(l + r) * divwidth, -(b + t) * divheight, dist, 1.0f);
    m.r[3] = vec4(0.0f, 0.0f, -dist * zn, 0.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
perspoffcenterrh(scalar l, scalar r, scalar b, scalar t, scalar zn, scalar zf)
{
    mat4 m = mat4::identity;
    scalar divwidth = 1.0f / (r - l);
    scalar divheight = 1.0f / (t - b);
    scalar dist = zf / (zn - zf);

    m.r[0] = vec4(2.0f * zn * divwidth, 0.0f, 0.0f, 0.0f);
    m.r[1] = vec4(0.0f, 2.0f * zn * divheight, 0.0f, 0.0f);
    m.r[2] = vec4((l + r) * divwidth, (b + t) * divheight, dist, -1.0f);
    m.r[3] = vec4(0.0f, 0.0f, dist * zn, 0.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
rotationaxis(const vec3& axis, scalar angle)
{
    __m128 norm = normalize(axis).vec;

    scalar sangle = Math::sin(angle);
    scalar cangle = Math::cos(angle);

    __m128 m1_c = _mm_set_ps1(1.0f - cangle);
    __m128 c = _mm_set_ps1(cangle);
    __m128 s = _mm_set_ps1(sangle);

    __m128 nn1 = _mm_shuffle_ps(norm,norm,_MM_SHUFFLE(3,0,2,1));
    __m128 nn2 = _mm_shuffle_ps(norm,norm,_MM_SHUFFLE(3,1,0,2));

    __m128 v = _mm_mul_ps(nn1,m1_c);
    v = _mm_mul_ps(nn2,v);

    __m128 nn3 = _mm_mul_ps(norm, m1_c);
    nn3 = _mm_mul_ps(norm, nn3);
    nn3 = _mm_add_ps(nn3, c);

    __m128 nn4 = _mm_mul_ps(norm,s);
    nn4 = _mm_add_ps(nn4, v);
    __m128 nn5 = _mm_mul_ps(s, norm);
    nn5 = _mm_sub_ps(v,nn5);

    v = _mm_and_ps(nn3, _mask_xyz);

    __m128 v1 = _mm_shuffle_ps(nn4,nn5,_MM_SHUFFLE(2,1,2,0));
    v1 = _mm_shuffle_ps(v1,v1,_MM_SHUFFLE(0,3,2,1));

    __m128 v2 = _mm_shuffle_ps(nn4,nn5,_MM_SHUFFLE(0,0,1,1));
    v2 = _mm_shuffle_ps(v2,v2,_MM_SHUFFLE(2,0,2,0));


    nn5 = _mm_shuffle_ps(v,v1,_MM_SHUFFLE(1,0,3,0));
    nn5 = _mm_shuffle_ps(nn5,nn5,_MM_SHUFFLE(1,3,2,0));

    mat4 m; 
    m.row0 = nn5;
    
    nn5 = _mm_shuffle_ps(v,v1,_MM_SHUFFLE(3,2,3,1));
    nn5 = _mm_shuffle_ps(nn5,nn5,_MM_SHUFFLE(1,3,0,2));
    m.row1 = nn5;

    v2 = _mm_shuffle_ps(v2,v,_MM_SHUFFLE(3,2,1,0));
    m.row2 = v2;

    m.row3 = _id_w;
    return m;

    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
rotationx(scalar angle)
{
    mat4 m = mat4::identity;

    scalar sangle = Math::sin(angle);
    scalar cangle = Math::cos(angle);

    m.m[1][1] = cangle;
    m.m[1][2] = sangle;

    m.m[2][1] = -sangle;
    m.m[2][2] = cangle;
    return m;   
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
rotationy(scalar angle)
{
    mat4 m = mat4::identity;

    scalar sangle = Math::sin(angle);
    scalar cangle = Math::cos(angle);

    m.m[0][0] = cangle;
    m.m[0][2] = -sangle;

    m.m[2][0] = sangle;
    m.m[2][2] = cangle;
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
rotationz(scalar angle)
{
    mat4 m = mat4::identity;

    scalar sangle = Math::sin(angle);
    scalar cangle = Math::cos(angle);

    m.m[0][0] = cangle;
    m.m[0][1] = sangle;

    m.m[1][0] = -sangle;
    m.m[1][1] = cangle;
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
rotationyawpitchroll(scalar yaw, scalar pitch, scalar roll)
{
    quat q = quatyawpitchroll(yaw, pitch, roll);
    return rotationquat(q);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
scaling(scalar scale)
{
    mat4 m = mat4::identity;
    m.r[0] = _mm_setr_ps(scale, 0.0f, 0.0f, 0.0f);
    m.r[1] = _mm_setr_ps(0.0f, scale, 0.0f, 0.0f);
    m.r[2] = _mm_setr_ps(0.0f, 0.0f, scale, 0.0f);
    m.r[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
scaling(scalar sx, scalar sy, scalar sz)
{
    mat4 m = mat4::identity;
    m.r[0] = _mm_setr_ps(sx, 0.0f, 0.0f, 0.0f);
    m.r[1] = _mm_setr_ps(0.0f, sy, 0.0f, 0.0f);
    m.r[2] = _mm_setr_ps(0.0f, 0.0f, sz, 0.0f);
    m.r[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
    
    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
scaling(const vec3& s)
{
    mat4 m = mat4::identity;
    m.r[0] = _mm_and_ps(s.vec, _mm_castsi128_ps(maskX));
    m.r[1] = _mm_and_ps(s.vec, _mm_castsi128_ps(maskY));
    m.r[2] = _mm_and_ps(s.vec, _mm_castsi128_ps(maskZ));    

    return m;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
translation(scalar x, scalar y, scalar z)
{
    mat4 m = mat4::identity;
    m.r[3] = _mm_set_ps(1.0f,z,y,x);
    return m;    
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
translation(const vec3& t)
{
    mat4 m = mat4::identity;
    m.r[3] = vec4(t.vec, 1.0f);
    return m;
}

//------------------------------------------------------------------------------
/**
    A short hand for creating a TRS (translation, rotation, scale) matrix from
    pos, rot and scale
*/
__forceinline mat4
trs(const vec3& position, const quat& rotation, const vec3& scale)
{
    return Math::affine(scale, rotation, position);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
transpose(const mat4& m)
{
    __m128 tmp3, tmp2, tmp1, tmp0;
    tmp0 = _mm_unpacklo_ps(m.r[0].vec, m.r[1].vec);
    tmp2 = _mm_unpacklo_ps(m.r[2].vec, m.r[3].vec);
    tmp1 = _mm_unpackhi_ps(m.r[0].vec, m.r[1].vec);
    tmp3 = _mm_unpackhi_ps(m.r[2].vec, m.r[3].vec);
    mat4 ret;
    ret.r[0] = _mm_movelh_ps(tmp0, tmp2);
    ret.r[1] = _mm_movehl_ps(tmp2, tmp0);
    ret.r[2] = _mm_movelh_ps(tmp1, tmp3);
    ret.r[3] = _mm_movehl_ps(tmp3, tmp1);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline mat4
skewsymmetric(const vec3& v)
{
    return {
        0,   -v.z,   v.y,   0,
        v.z,  0,    -v.x,   0,
       -v.y,  v.x,   0,     0,
        0,    0,     0,     0,
    };
}

} // namespace Math
//------------------------------------------------------------------------------

