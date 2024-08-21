//------------------------------------------------------------------------------
//  util.fxh
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef UTIL_FXH
#define UTIL_FXH
#include "lib/std.fxh"

const float depthScale = 100.0f;


//------------------------------------------------------------------------------
/**
    Unpack signed short UVs to float
*/
vec2
UnpackUV(ivec2 packedUv)
{
    return packedUv * (1.0f / 1000.0f);
}

//------------------------------------------------------------------------------
/**
    Encode 2 values in the range 0..1 into a 4-channel vector. Used
    for encoding PSSM-depth values into a 32-bit-rgba value.
*/
vec4
Encode2(vec2 inVals)
{
    return vec4(inVals.x, fract(inVals.x * 256.0), inVals.y, fract(inVals.y * 256.0));
}

//------------------------------------------------------------------------------
/**
    Decode 2 values encoded by Encode2().
*/
vec2
Decode2(vec4 inVals)
{
    return vec2(inVals.x + (inVals.y / 256.0), inVals.z + (inVals.w / 256.0));
}

//------------------------------------------------------------------------------
/**
    Unpack a UB4N packedData normal.
*/
vec3
UnpackNormal(vec3 packedDataNormal)
{
    return (packedDataNormal * 2.0) - 1.0;
}

//------------------------------------------------------------------------------
/**
    Unpack a packedData vertex normal.
*/
vec4
UnpackNormal4(vec4 packedDataNormal)
{
    return vec4((packedDataNormal.xyz * 2.0) - 1.0, 1.0f);
}

//------------------------------------------------------------------------------
/**
    Unpack a 4.12 packedData texture coord.
*/
vec2
UnpackUv(vec2 packedDataUv)
{
    return (packedDataUv / 8192.0);
}

//------------------------------------------------------------------------------
/**
    Unpack a skin weight vertex component. Since the packing looses some
    precision we need to re-normalize the weights.
*/
vec4
UnpackWeights(vec4 weights)
{
    return (weights / dot(weights, vec4(1.0, 1.0, 1.0, 1.0)));
}

//------------------------------------------------------------------------------
/**
    Calculate cubic weights
*/
vec4
CubicWeights(float v)
{
    vec4 n = vec4(1, 2, 3, 4) - v;
    vec4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4 * s.x;
    float z = s.z - 4 * s.y + 6 * s.x;
    float w = 6 - x - y - z;
    return vec4(x, y, z, w) / 6.0f;
}

//------------------------------------------------------------------------------
/**
    Sample texture using bicubic sampling
*/
vec3
SampleCubic(texture2D tex, sampler samp, vec4 res, vec2 pixel, uint mip)
{
    vec2 coords = pixel * res.xy - 0.5f;
    vec2 fxy = fract(coords);
    coords -= fxy;

    vec4 xcubic = CubicWeights(fxy.x);
    vec4 ycubic = CubicWeights(fxy.y);

    vec4 c = coords.xxyy + vec2(-0.5f, 1.5f).xyxy;
    vec4 s = vec4(xcubic.xz + xcubic.yw, ycubic.xz + ycubic.yw);
    vec4 offset = min(max(vec4(0), c + vec4(xcubic.yw, ycubic.yw) / s), res.xxyy);

    offset *= res.zzww;
    vec3 sample0 = textureLod(sampler2D(tex, samp), offset.xz, mip).rgb;
    vec3 sample1 = textureLod(sampler2D(tex, samp), offset.yz, mip).rgb;
    vec3 sample2 = textureLod(sampler2D(tex, samp), offset.xw, mip).rgb;
    vec3 sample3 = textureLod(sampler2D(tex, samp), offset.yw, mip).rgb;
    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);
    return lerp(lerp(sample3, sample2, sx), lerp(sample1, sample0, sx), sy);
}

#define USE_SRGB 1
#if USE_SRGB
#define EncodeHDR(x) x
#define EncodeHDR4(x) x
#define DecodeHDR(x) x
#define DecodeHDR4(x) x
#else
//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
vec4
EncodeHDR(in vec4 rgba)
{
    return rgba * vec4(0.5, 0.5, 0.5, 1.0);
}

//------------------------------------------------------------------------------
/**
    Scale down pseudo-HDR-value into RGB8.
*/
vec4
EncodeHDR4(in vec4 rgba)
{
    return rgba * vec4(0.5, 0.5, 0.5, 0.5);
}

//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/
vec4
DecodeHDR(in vec4 rgba)
{
    return rgba * vec4(2.0f, 2.0f, 2.0f, 1.0f);
}


//------------------------------------------------------------------------------
/**
    Scale up pseudo-HDR-value encoded by EncodeHDR() in shaders.fx.
*/
vec4
DecodeHDR4(in vec4 rgba)
{
    return rgba * vec4(2.0f, 2.0f, 2.0f, 2.0f);
}
#endif

//------------------------------------------------------------------------------
/**
*/
vec3
RGBToXYZ(vec3 rgb)
{
    const mat3 RGB_2_XYZ = mat3(
        0.4124564, 0.2126729, 0.0193339,
        0.3575761, 0.7151522, 0.1191920,
        0.1804375, 0.0721750, 0.9503041
    );
    return RGB_2_XYZ * rgb;
}

//------------------------------------------------------------------------------
/**
*/
vec3
XYZToRGB(vec3 xyz)
{
    const mat3 XYZ_2_RGB = mat3(
        3.2404542, -0.9692660, 0.0556434,
        -1.5371385, 1.8760108, -0.2040259,
        -0.4985314, 0.0415560, 1.0572252
    );
    return XYZ_2_RGB * xyz;
}

//------------------------------------------------------------------------------
/**
*/
vec3
RGBToXYY(vec3 rgb)
{
    vec3 xyz = RGBToXYZ(rgb);
    float Y = xyz.y;
    float x = xyz.x / (xyz.x + xyz.y + xyz.z);
    float y = xyz.y / (xyz.x + xyz.y + xyz.z);
    return vec3(x, y, Y);
}

//------------------------------------------------------------------------------
/**
*/
vec3
XYYToRGB(vec3 xyY)
{
    float y = xyY.z;
    float x = y * xyY.x / xyY.y;
    float z = y * (1.0f - xyY.x - xyY.y) / xyY.y;
    return XYZToRGB(vec3(x, y, z));
}

const float MiddleGrey = 0.5f;
const float Key = 0.3f;
const vec3 Luminance = vec3(0.2126f, 0.7152f, 0.0722f);
//const vec4 Luminance = vec4(0.299f, 0.587f, 0.114f, 0.0f);

//------------------------------------------------------------------------------
/**
*/
float 
Greyscale(in vec4 color)
{
    return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

//------------------------------------------------------------------------------
/**
    Calculates HDR tone mapping
*/
vec3
ToneMap(vec3 xyY, float lp, float maxLum)
{
    const float WhitePoint = 4.0;

    // apply reinhard2 tonemapping
    xyY.z = (lp * (1.0f + lp / (WhitePoint * WhitePoint))) / (1.0f + lp);

    return xyY;
}

//------------------------------------------------------------------------------
/**
    From GPUGems3: "Don't know why this isn't in the standard library..."
*/
float
linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0f, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
float
sqr(float f)
{
    return f * f;
}

//------------------------------------------------------------------------------
/**
*/
uvec2
unpack_2u16(uint value)
{
    return uvec2(value & 0xFFFF, (value >> 16) & 0xFFFF);
}

#define PI 3.14159265
#define ONE_OVER_PI 1/PI
#define PI_OVER_FOUR PI/4.0f
#define PI_OVER_TWO PI/2.0f

//-------------------------------------------------------------------------------------------------------------
/**
    Logarithmic filtering
*/
float log_conv( float x0, float x, float y0, float y )
{
    return (x + log(x0 + (y0 * exp(y - x))));
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel size.
*/
vec2
GetPixelSize(in sampler2D tex)
{
    vec2 size = textureSize(tex, 0);
    size = vec2(1.0f) / size;
    return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel size.
*/
vec2
GetScaledUVs(in vec2 uvs, in sampler2D tex, in vec2 dimensions)
{
    vec2 texSize = textureSize(tex, 0);
    uvs = uvs * (dimensions / texSize);
    return uvs;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute UV from pixel coordinate and texture
*/
vec2
GetUV(in ivec2 pixel, in sampler2D tex)
{
    vec2 size = textureSize(tex, 0);
    size = pixel / size;
    return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute pixel from UV
*/
ivec2
GetPixel(in vec2 uv, in sampler2D tex)
{
    ivec2 size = textureSize(tex, 0);
    size = ivec2(uv * size);
    return size;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Compute texture ratio to current pixel size
*/
vec2
GetTextureRatio(in sampler2D tex, vec2 pixelSize)
{
    ivec2 size = textureSize(tex, 0);
    vec2 currentTextureSize = vec2(1.0f) / size;
    return size / currentTextureSize;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Flips y in vec2, useful for opengl conversion of uv corods
*/
vec2
FlipY(vec2 uv)
{
    return vec2(uv.x, 1.0f - uv.y);
}

//------------------------------------------------------------------------------
/**
*/
mat3
PlaneTBN(vec3 normal)
{
    vec3 tangent = cross(normal.xyz, vec3(0, 0, 1));
    tangent = normalize(cross(normal.xyz, tangent));
    vec3 binormal = normalize(cross(normal.xyz, tangent));
    return mat3(tangent, binormal, normal.xyz);
}

//------------------------------------------------------------------------------
/**
*/
mat3
TangentSpace(vec3 tangent, vec3 normal, float sign)
{
    vec3 tan = normalize(tangent);
    vec3 norm = normalize(normal);
    vec3 bin = cross(norm, tan) * sign;
    return mat3(tan, bin, norm);
}

//------------------------------------------------------------------------------
/**
*/
vec3 
UnpackBC5Normal(vec2 normalMapComponents)
{
    vec3 normal = vec3(0, 0, 0);
    normal.xy = (normalMapComponents * 2.0f) - 1.0f;
    normal.z = saturate(sqrt(1.0f - dot(normal.xy, normal.xy)));
    return normal;
}

//------------------------------------------------------------------------------
/**
*/
vec3
TangentSpaceNormal(vec2 normalMapComponents, mat3 tbn)
{
    return tbn * UnpackBC5Normal(normalMapComponents);
}

//-------------------------------------------------------------------------------------------------------------
/**
*/
float
LinearizeDepth(float depth, vec2 focalLength)
{
    return (focalLength.x * focalLength.y) / (depth * (focalLength.x - focalLength.y) + focalLength.y);
}

//-------------------------------------------------------------------------------------------------------------
/**
*/
float
DelinearizeDepth(float depth, vec2 focalLength)
{
    return -((focalLength.x + focalLength.y) * depth - (2 * focalLength.x)) / ((focalLength.x - focalLength.y) * depth);
}

//-------------------------------------------------------------------------------------------------------------
/**
    Convert pixel to normalized [0,1] space
*/
vec2
PixelToNormalized(in vec2 screenCoord, in vec2 pixelSize)
{
    return screenCoord.xy * pixelSize.xy;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Convert pixel to projection space
*/
vec4
PixelToProjection(vec2 screenCoord, float depth)
{
    // we use DX depth range [0,1], for GL where depth is [-1,1], we would need depth * 2 - 1 too
    return vec4(screenCoord * 2.0f - 1.0f, depth, 1.0f);
}

//-------------------------------------------------------------------------------------------------------------
/**
    Convert pixel to view space
*/
vec4
PixelToView(vec2 screenCoord, float depth, mat4 invProjection)
{
    vec4 projectionSpace = PixelToProjection(screenCoord, depth);
    vec4 viewSpace = invProjection * projectionSpace;
    viewSpace /= viewSpace.w;
    return viewSpace;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Convert pixel to view space
*/
vec4
PixelToWorld(vec2 screenCoord, float depth, mat4 invView, mat4 invProjection)
{
    vec4 viewSpace = PixelToView(screenCoord, depth, invProjection);
    return invView * viewSpace;
}

//-------------------------------------------------------------------------------------------------------------
/**
    Convert view space to world space
*/
vec4
ViewToWorld(const vec4 viewSpace, mat4 invView)
{
    return invView * viewSpace;
}

//------------------------------------------------------------------------------
/**
    Get position element from matrix
*/
vec3
GetPosition(mat4x4 transform)
{
    return transform[2].xyz;
}

//------------------------------------------------------------------------------
/**
    Unpack a 1D index into a 3D index
*/
uint3
Unpack1DTo3D(uint index1D, uint width, uint height)
{
    uint i = index1D % width;
    uint j = index1D % (width * height) / width;
    uint k = index1D / (width * height);

    return uint3(i, j, k);
}

//------------------------------------------------------------------------------
/**
    Pack a 3D index into a 1D array index
*/
uint
Pack3DTo1D(uint3 index3D, uint width, uint height)
{
    return index3D.x + (width * (index3D.y + height * index3D.z));
}

//------------------------------------------------------------------------------
/**
    Converts a linear sequence to a morton curve 8x8 access pattern
*/
uvec2
MortonCurve8x8(uint idx)
{
    // yeah... don't ask
    uint x = bitfieldExtract(idx, 2, 3);
    x = bitfieldInsert(x, idx, 0, 1);

    uint y = bitfieldExtract(idx, 3, 3);
    uint a = bitfieldExtract(idx, 1, 2);
    y = bitfieldInsert(y, a, 0, 2);

    return uvec2(x, y);
}

//------------------------------------------------------------------------------
/**
*/
bool 
IntersectLineWithPlane(vec3 lineStart, vec3 lineEnd, vec4 plane, out vec3 intersect)
{
    vec3 ab = lineEnd - lineStart;
    float t = (plane.w - dot(plane.xyz, lineStart)) / dot(plane.xyz, ab);
    bool ret = (t >= 0.0f && t <= 1.0f);
    intersect = vec3(0, 0, 0);
    if (ret)
    {
        intersect = lineStart + t * ab;
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
    note: from https://www.shadertoy.com/view/4djSRW
    This set suits the coords of of 0-1.0 ranges..
*/
#define MOD3 vec3(443.8975,397.2973, 491.1871)

//------------------------------------------------------------------------------
/**
*/
float 
hash11(float p)
{
    vec3 p3 = fract(vec3(p) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//------------------------------------------------------------------------------
/**
*/
float 
hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//------------------------------------------------------------------------------
/**
*/
vec3 
hash32(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * MOD3);
    p3 += dot(p3, p3.yxz + 19.19);
    return fract(vec3((p3.x + p3.y) * p3.z, (p3.x + p3.z) * p3.y, (p3.y + p3.z) * p3.x));
}

const int m = 1540483477;
//------------------------------------------------------------------------------
/**
    Murmur hash function
*/
float 
murmur(int k)
{
    int h = k ^ 1;

    k *= m;
    k ^= k >> 24;
    k *= m;

    h *= m;
    h ^= k;

    return float(h);
}

//------------------------------------------------------------------------------
/**
    Discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3
*/
vec3
random3(vec3 c) {

    vec3 r;
    float c1 = dot(c, vec3(17.0f, 59.4f, 15.0f));
    r.x = fract(murmur(int(c.x * c1)) / 512.0f);
    r.y = fract(murmur(int(c.y * c1)) / 512.0f);
    r.z = fract(murmur(int(c.z * c1)) / 512.0f);
    return r - 0.5f;
}

//------------------------------------------------------------------------------
/**
    Source: https://www.shadertoy.com/view/XsX3zB

    With modifications to the random3 function using a murmur hash instead of trigonometry
*/

const float F3 = 0.3333333;
const float G3 = 0.1666667;
float 
simplex3D(vec3 p)
{
    /* 1. find current tetrahedron T and it's four vertices */
    /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
    /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/

    /* calculate s and x */
    vec3 s = floor(p + dot(p, vec3(F3)));
    vec3 x = p - s + dot(s, vec3(G3));

    /* calculate i1 and i2 */
    vec3 e = step(vec3(0.0f), x - x.yzx);
    vec3 i1 = e * (1.0f - e.zxy);
    vec3 i2 = 1.0f - e.zxy * (1.0f - e);

    /* x1, x2, x3 */
    vec3 x1 = x - i1 + G3;
    vec3 x2 = x - i2 + 2.0f * G3;
    vec3 x3 = x - 1.0f + 3.0f * G3;

    /* 2. find four surflets and store them in d */
    vec4 w, d;

    /* calculate surflet weights */
    w.x = dot(x, x);
    w.y = dot(x1, x1);
    w.z = dot(x2, x2);
    w.w = dot(x3, x3);

    /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
    w = max(0.6f - w, 0.0f);

    /* calculate surflet components */
    d.x = dot(random3(s), x);
    d.y = dot(random3(s + i1), x1);
    d.z = dot(random3(s + i2), x2);
    d.w = dot(random3(s + 1.0f), x3);

    /* multiply d by w^4 */
    w *= w;
    w *= w;
    d *= w;

    /* 3. return the sum of the four surflets */
    return dot(d, vec4(52.0f));
}

//------------------------------------------------------------------------------
/**
*/
const mat3 rot1 = mat3(-0.37, 0.36, 0.85, -0.14, -0.93, 0.34, 0.92, 0.01, 0.4);
const mat3 rot2 = mat3(-0.55, -0.39, 0.74, 0.33, -0.91, -0.24, 0.77, 0.12, 0.63);
const mat3 rot3 = mat3(-0.71, 0.52, -0.47, -0.08, -0.72, -0.68, -0.7, -0.45, 0.56);
float
simplex3D_fractal(vec3 m)
{
    return  0.5333333f * simplex3D(m * rot1)
        + 0.2666667f * simplex3D(2.0f * m * rot2)
        + 0.1333333f * simplex3D(4.0f * m * rot3)
        + 0.0666667f * simplex3D(8.0f * m);
}

//------------------------------------------------------------------------------
/**
    Source: https://www.shadertoy.com/view/Msf3WH
*/
vec2
hash(vec2 p)
{
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

//------------------------------------------------------------------------------
/**
    Source: https://www.shadertoy.com/view/Msf3WH
*/
float 
noise(in vec2 p)
{
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

    vec2  i = floor(p + (p.x + p.y) * K1);
    vec2  a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2  o = vec2(m, 1.0 - m);
    vec2  b = a - o + K2;
    vec2  c = a - 1.0 + 2.0 * K2;
    vec3  h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3  n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
    return dot(n, vec3(70.0));
}

//------------------------------------------------------------------------------
#endif
