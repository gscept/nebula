#pragma once
//------------------------------------------------------------------------------
/**
    @file scalar.h

    Nebula's scalar datatype.

    NOTE: do not add CRT math function calls to this call, but instead
    into the platform specific headers 
    functions are called and must be placed into a .cc file, not into the
    header.

    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/



namespace Math
{
#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif

// the half circle
#ifndef N_PI
#define N_PI (Math::scalar(PI))
#endif
// the whole circle
#ifndef N_PI_DOUBLE
#define N_PI_DOUBLE (Math::scalar(6.283185307179586476925286766559))
#endif
// a quarter circle
#ifndef N_PI_HALF
#define N_PI_HALF (Math::scalar(1.5707963267948966192313216916398f))
#endif
// ---HOTFIXED

#ifndef TINY
#define TINY (0.0000001f)
#endif
#define N_TINY TINY

typedef float scalar;
typedef float float32;
typedef double float64;

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif
// the half circle
#ifndef N_PI
#define N_PI (Math::scalar(3.1415926535897932384626433832795028841971693993751))
#endif

struct float2
{
    union
    {
        struct { scalar x, y; };
        scalar v[2];
    };
};

struct float3
{
    union
    {
        struct { scalar x, y, z; };
        scalar v[3];
    };
};

struct float4
{
    union
    {
        struct { scalar x, y, z, w; };
        scalar v[4];
    };
};

struct int2
{
    union
    {
        struct { int x, y; };
        int v[2];
    };
};

struct int3
{
    union
    {
        struct { int x, y, z; };
        int v[3];
    };
};

struct int4
{
    union
    {
        struct { int x, y, z, w; };
        int v[4];
    };
};

struct uint2
{
    union
    {
        struct { unsigned int x, y; };
        unsigned int v[2];
    };
};

struct uint3
{
    union
    {
        struct { unsigned int x, y, z; };
        unsigned int v[3];
    };
};

struct uint4
{
    union
    {
        struct { unsigned int x, y, z, w; };
        unsigned int v[4];
    };
};

struct byte4u
{
    union
    {
        struct { ubyte x, y, z, w; };
        unsigned int v;
    };
};

struct byte4
{
    union
    {
        struct { byte x, y, z, w; };
        unsigned int v;
    };
};

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between 0 and 1.
*/
__forceinline scalar 
rand()
{
    return scalar(::rand()) / scalar(RAND_MAX);
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between min and max.
*/
__forceinline scalar 
rand(scalar min, scalar max)
{
    scalar unit = scalar(::rand()) / RAND_MAX;
    scalar diff = max - min;
    return min + unit * diff;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
sin(scalar x)
{
    return sinf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
cos(scalar x)
{
    return cosf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
tan(scalar x)
{
    return tanf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
asin(scalar x)
{
    return asinf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
acos(scalar x)
{
    return acosf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
atan(scalar x)
{
    return atanf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar 
sqrt(scalar x)
{
    return sqrtf(x);
}

//------------------------------------------------------------------------------
/**
    Chop float to int.
*/
__forceinline int 
fchop(scalar f)
{
    /// @todo type cast to int is slow!
    return int(f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
fmod(scalar x, scalar y)
{
    return fmodf(x, y);
}

//------------------------------------------------------------------------------
/**
    Normalize an angular value into the range rad(0) to rad(360).
    FIXME : seems that the xna version limits from -pi to pi, not 0 .. 2pi. 
            will copy behaviour despite what description says
*/
__forceinline scalar 
modangle(scalar a) 
{
#if 0
    static const scalar rev = scalar(6.283185307179586476925286766559);
    return n_fmod(a,rev);
#else
    static const scalar REVOLUTION = scalar(6.283185307179586476925286766559);
    a += N_PI;
    scalar temp = fabsf(a);
    temp = temp - floorf(temp/REVOLUTION) *  REVOLUTION;
    temp -= N_PI;
    temp = a<0 ? -temp:temp;    
    if(temp < scalar(-N_PI)) temp += REVOLUTION;
    if(temp >= scalar(N_PI)) temp -= REVOLUTION;
    return temp;
#endif
}

//------------------------------------------------------------------------------
/**
    log2() function.
*/
__forceinline scalar 
log2(scalar f) 
{ 
    return log2f(f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
exp2(scalar x)
{
    return exp2f(x);
}

//------------------------------------------------------------------------------
/**
    get logarithm of x
*/
__forceinline scalar
log(scalar x)
{
    return logf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
exp(scalar x)
{
    return expf(x);
}

//------------------------------------------------------------------------------
/**
    Round float to integer.
*/
__forceinline int 
frnd(scalar f)
{
    return fchop(floorf(f + 0.5f));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
pow(scalar x, scalar y)
{
    return powf(x, y);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point equality check
*/
__forceinline bool
fequal(scalar f0, scalar f1, scalar tol)
{
    scalar f = f0 - f1;
    return ((f > (-tol)) && (f < tol));
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
__forceinline TYPE
max(TYPE a, TYPE b)
{
    return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE0, typename ...TYPEN>
__forceinline TYPE0
max(TYPE0 first, TYPE0 second, TYPEN... rest)
{
    return first > second ? max(first, std::forward<TYPEN>(rest)...) : max(second, std::forward<TYPEN>(rest)...);
}

//------------------------------------------------------------------------------
/**
    branchless max for uint32
*/
template<>
__forceinline unsigned int
max(unsigned int a, unsigned int b)
{
    return a ^ ((a ^ b) & -(a < b));
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
__forceinline TYPE
min(TYPE a, TYPE b)
{
    return (a < b) ? a : b;
}

//------------------------------------------------------------------------------
/**
    branchless min for uints
*/
template<>
__forceinline unsigned int
min(unsigned int a, unsigned int b)
{
    return b ^ ((a ^ b) & -(a < b));
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE0, typename ...TYPEN>
__forceinline TYPE0
min(TYPE0 first, TYPE0 second, TYPEN... rest)
{
    return first < second ? min(first, std::forward<TYPEN>(rest)...) : min(second, std::forward<TYPEN>(rest)...);
}

//------------------------------------------------------------------------------
/**
*/

__forceinline scalar
abs(scalar a)
{
    return (a < 0.0f) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline double
abs(double a)
{
    return (a < 0.0) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
abs(int a)
{
    return (a < 0) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
sgn(scalar a)
{
    return (a < 0.0f) ? -1.0f : 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline constexpr scalar
deg2rad(scalar d)
{
    return (scalar)((d * N_PI) / 180.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline constexpr scalar
rad2deg(scalar r)
{
    return (scalar)((r * 180.0f) / N_PI);
}

//------------------------------------------------------------------------------
/**
    Float clamping.
*/
__forceinline float
clamp(float val, float minVal, float maxVal)
{
    if (val < minVal)      return minVal;
    else if (val > maxVal) return maxVal;
    else return val;
}

//------------------------------------------------------------------------------
/**
    int clamping.
*/
__forceinline int64_t
clamp(int64_t val, int64_t minVal, int64_t maxVal)
{
    if (val < minVal)      return minVal;
    else if (val > maxVal) return maxVal;
    else return val;
}

//------------------------------------------------------------------------------
/**
    int clamping.
*/
__forceinline int32_t
clamp(int32_t val, int32_t minVal, int32_t maxVal)
{
    if (val < minVal)      return minVal;
    else if (val > maxVal) return maxVal;
    else return val;
}

//------------------------------------------------------------------------------
/**
    Floating point ceiling
*/
__forceinline float
ceil(float val)
{
    return ceilf(val);
}

//------------------------------------------------------------------------------
/**
    Floating point flooring
*/
__forceinline float
floor(float val)
{
    return floorf(val);
}

//------------------------------------------------------------------------------
/**
    Floating point rounding
*/
__forceinline float
round(float val)
{
    return roundf(val);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point less-then check.
*/
__forceinline bool
fless(scalar f0, scalar f1, scalar tol)
{
    return ((f0 - f1) < tol);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point greater-then check.
*/
__forceinline bool
fgreater(scalar f0, scalar f1, scalar tol)
{
    return ((f0 - f1) > tol);
}

//------------------------------------------------------------------------------
/**
    Saturate a value (clamps between 0.0f and 1.0f)
*/
__forceinline scalar
saturate(scalar val)
{
    if (val < 0.0f)      return 0.0f;
    else if (val > 1.0f) return 1.0f;
    else return val;
}

//------------------------------------------------------------------------------
/**
Saturate a value (clamps between 0.0f and 1.0f)
*/
__forceinline double
saturate(double val)
{
    if (val < 0.0)      return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

//------------------------------------------------------------------------------
/**
    Linearly interpolate between 2 values: ret = x + l * (y - x)
*/
__forceinline float
lerp(float x, float y, float l)
{
    return x + l * (y - x);
}

//------------------------------------------------------------------------------
/**
    Get angular distance.
*/
__forceinline scalar
angulardistance(scalar from, scalar to)
{
    scalar normFrom = modangle(from);
    scalar normTo   = modangle(to);
    scalar dist = normTo - normFrom;
    if (dist < deg2rad(-180.0f))
    {
        dist += deg2rad(360.0f);
    }
    else if (dist > deg2rad(180.0f))
    {
        dist -= deg2rad(360.0f);
    }
    return dist;
}

//------------------------------------------------------------------------------
/**
    Returns true if the input scalar is denormalized (#DEN)
*/
__forceinline bool
isdenormal(scalar s)
{
#if __GNUC__
    union { scalar s; unsigned int u; } pun;
    pun.s = s;
    return ((pun.u&0x7f800000)==0);
#else
    return (((*(unsigned int*)&s)&0x7f800000)==0);
#endif
}

//------------------------------------------------------------------------------
/**
    Returns 0 if scalar is denormal.
*/
__forceinline float
undenormalize(scalar s)
{
    if (isdenormal(s))
    {
        return 0.0f;
    }
    else
    {
        return s;
    }
}

//------------------------------------------------------------------------------
/**
    test of nearly equal given a tolerance (epsilon)
*/
__forceinline bool
nearequal(scalar a, scalar b, scalar epsilon)
{
    return abs(a - b) <= epsilon;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
cot(scalar x)
{
    return scalar(1.0) / tan(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
smoothstep(scalar edge0, scalar edge1, scalar x)
{
    // Scale, bias and saturate x to 0..1 range
    x = saturate((x - edge0) / (edge1 - edge0)); 
    // Evaluate polynomial
    return x*x*(3-2*x);     
}

//------------------------------------------------------------------------------
/**
    Return a pseudo integer random number between min and max.
*/
__forceinline int 
irand(int min, int max)
{   
    int range = max - min + 1;
    int unit = ::rand() % range;
    return min + unit;
}

//------------------------------------------------------------------------------
/**
    Returns the position of the most significant bit of the number
*/
__forceinline int
mostsignificant(unsigned int val)
{
#ifdef WIN32
    unsigned long ret;
    bool res = _BitScanReverse(&ret, val);
    ret = res ? ret : 0;
    return ret + 1;
#else
    unsigned long ret;
    ret = __builtin_clz(val);
    return ret + 1;
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline unsigned int
align(unsigned int alignant, unsigned int alignment)
{
    return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uintptr_t
alignptr(uintptr_t alignant, uintptr_t alignment)
{
    return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline unsigned int
align_down(unsigned int alignant, unsigned int alignment)
{
    return (alignant / alignment * alignment);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uintptr_t
align_downptr(uintptr_t alignant, uintptr_t alignment)
{
    return (alignant / alignment * alignment);
}

//------------------------------------------------------------------------------
/**
    Integer division with rounding
*/
__forceinline unsigned int
divandroundup(unsigned int dividend, unsigned int divider)
{
    return (dividend % divider != 0) ? (dividend / divider + 1) : (dividend / divider);
}

//------------------------------------------------------------------------------
/**
    Rounds up to next power of 2
*/
__forceinline unsigned int
roundtopow2(unsigned int val)
{
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val++;
    return val;
}

//------------------------------------------------------------------------------
/**
    fast random generator based on xorshift+
*/
struct randxorstate
{
    uint64 x[2];
};
__forceinline uint64
randxor(randxorstate& state)
{
    uint64_t t = state.x[0];
    uint64_t const s = state.x[1];
    state.x[0] = s;
    t ^= t << 23;		// a
    t ^= t >> 18;		// b -- Again, the shifts and the multipliers are tunable
    t ^= s ^ (s >> 5);	// c
    state.x[1] = t;
    return t + s;
}

} // namespace Math

//------------------------------------------------------------------------------
/**
    Convert degrees to radians
*/
inline long double
operator"" _rad(long double deg)
{
    return Math::deg2rad(deg);
}

//------------------------------------------------------------------------------
/**
*/
inline long double
operator"" _rad(unsigned long long deg)
{
    return Math::deg2rad(deg);
}

//------------------------------------------------------------------------------
/**
    Convert radians to degrees
*/
inline long double
operator"" _deg(long double rad)
{
    return Math::rad2deg(rad);
}

//------------------------------------------------------------------------------
/**
*/
inline long double
operator"" _deg(unsigned long long deg)
{
    return Math::rad2deg(deg);
}
//------------------------------------------------------------------------------
