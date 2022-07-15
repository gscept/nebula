//------------------------------------------------------------------------------
//  @file staticui.fx
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/techniques.fxh" 
#include "lib/sdf.fxh"
#include "lib/shared.fxh"

#include "lib/fullscreen.fxh"

group(DYNAMIC_OFFSET_GROUP) constant PerDrawState [ string Visibility = "VS|PS"; ]
{
    mat4 Transform;
    mat4 Clip[8];
    vec4 Scalar4[2];
    vec4 Vector[8];
    uint ClipSize;

    textureHandle texture1;
    textureHandle texture2;
};

group(DYNAMIC_OFFSET_GROUP) sampler_state UISampler
{

};

vec4 sRGBToLinear(vec4 val) { return vec4(val.xyz * (val.xyz * (val.xyz * 0.305306011 + 0.682171111) + 0.012522878), val.w); }

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain1(
    [slot = 0] in vec2 pos
    , [slot = 1] in uvec4 color
    , [slot = 2] in vec2 uv
    , out vec4 Color
    , out vec2 ObjUv
)
{
    ObjUv = uv;
    gl_Position = Transform * vec4(pos, 0, 1);
    Color = vec4(color) / 255.0f;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain2(
    [slot = 0] in vec2 pos
    , [slot = 1] in uvec4 color
    , [slot = 2] in vec2 uv
    , [slot = 3] in vec2 objUv
    , [slot = 4] in vec4 extra0
    , [slot = 5] in vec4 extra1
    , [slot = 6] in vec4 extra2
    , [slot = 7] in vec4 extra3
    , [slot = 8] in vec4 extra4
    , [slot = 9] in vec4 extra5
    , [slot = 10] in vec4 extra6
    , out vec4 Color
    , out vec2 Uv
    , out vec2 ObjUv
    , out vec4 Extra0
    , out vec4 Extra1
    , out vec4 Extra2
    , out vec4 Extra3
    , out vec4 Extra4
    , out vec4 Extra5
    , out vec4 Extra6
)
{
    ObjUv = objUv;
    gl_Position = Transform * vec4(pos, 0, 1);
    Color = vec4(color) / 255.0f;
    Uv = uv;
    Extra0 = extra0;
    Extra1 = extra1;
    Extra2 = extra2;
    Extra3 = extra3;
    Extra4 = extra4;
    Extra5 = extra5;
    Extra6 = extra6;
}

//------------------------------------------------------------------------------
/**
*/
void 
Unpack(vec4 x, out vec4 a, out vec4 b)
{
    const float s = 65536.0;
    a = floor(x / s);
    b = floor(x - a * s);
}

//------------------------------------------------------------------------------
/**
*/
vec2 
TransformAffine(vec2 val, vec2 a, vec2 b, vec2 c) 
{
    return val.x * a + val.y * b + c;
}

#define AA_WIDTH 0.354

//------------------------------------------------------------------------------
/**
*/
float 
Antialias(in float d, in float width, in float median)
{
    return smoothstep(median - width, median + width, d);
}

//------------------------------------------------------------------------------
/**
*/
vec4
ApplyClip(vec4 clipColor, vec2 objUv)
{
    vec4 color = clipColor;
    for (uint i = 0u; i < ClipSize; i++)
    {
        mat4 data = Clip[i];
        vec2 origin = data[0].xy;
        vec2 size = data[0].zw;
        vec4 radii_x, radii_y;
        Unpack(data[1], radii_x, radii_y);
        bool inverse = bool(data[3].z);

        vec2 p = objUv;
        p = TransformAffine(p, data[2].xy, data[2].zw, data[3].xy);
        p -= origin;

        float d_clip = sdRoundRect(p, size, radii_x, radii_y) * (inverse ? -1.0 : 1.0);
        float alpha = Antialias(-d_clip, AA_WIDTH, 0.0);
        color = vec4(color.rgb * alpha, color.a * alpha);
    }
    return color;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psFill1(
    in vec4 color
    , in vec2 objUv
    , [color0] out vec4 Color
)
{
    Color = ApplyClip(color, objUv);
}

//------------------------------------------------------------------------------
/**
    Extraction functions
*/
uint FillType(vec4 data0) { return uint(data0.x + 0.5); }
vec4 TileRectUV() { return Vector[0]; }
vec2 TileSize() { return Vector[1].zw; }
vec2 PatternTransformA() { return Vector[2].xy; }
vec2 PatternTransformB() { return Vector[2].zw; }
vec2 PatternTransformC() { return Vector[3].xy; }
uint Gradient_NumStops(vec4 data0) { return uint(data0.y + 0.5); }
bool Gradient_IsRadial(vec4 data0) { return bool(uint(data0.z + 0.5)); }
float Gradient_R0(vec4 data1) { return data1.x; }
float Gradient_R1(vec4 data1) { return data1.y; }
vec2 Gradient_P0(vec4 data1) { return data1.xy; }
vec2 Gradient_P1(vec4 data1) { return data1.zw; }
float SDFMaxDistance(vec4 data0) { return data0.y; }

// Uniform Accessor Functions
float Scalar(uint i) { if (i < 4u) return Scalar4[0][i]; else return Scalar4[1][i - 4u]; }

//------------------------------------------------------------------------------
/**
*/
vec4
FillSolid(vec4 color)
{
    return color;
}

//------------------------------------------------------------------------------
/**
*/
vec4
FillImage(vec2 uv, vec4 color)
{
    return sample2D(texture1, UISampler, uv) * color;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendOverlay(vec3 src, vec3 dest)
{
    vec3 col;
    for (int i = 0; i < 3; ++i)
        col[i] = dest[i] < 0.5 ? (2.0 * dest[i] * src[i]) : (1.0 - 2.0 * (1.0 - dest[i]) * (1.0 - src[i]));
    return col;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendColorDodge(vec3 src, vec3 dest)
{
    vec3 col;
    for (int i = 0; i < 3; ++i)
        col[i] = (src[i] == 1.0) ? src[i] : min(dest[i] / (1.0 - src[i]), 1.0);
    return col;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendColorBurn(vec3 src, vec3 dest)
{
    vec3 col;
    for (int i = 0; i < 3; ++i)
        col[i] = (src[i] == 0.0) ? src[i] : max((1.0 - ((1.0 - dest[i]) / src[i])), 0.0);
    return col;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendHardLight(vec3 src, vec3 dest)
{
    vec3 col;
    for (int i = 0; i < 3; ++i)
        col[i] = dest[i] < 0.5 ? (2.0 * dest[i] * src[i]) : (1.0 - 2.0 * (1.0 - dest[i]) * (1.0 - src[i]));
    return col;
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendSoftLight(vec3 src, vec3 dest)
{
    vec3 col;
    for (int i = 0; i < 3; ++i)
        col[i] = (src[i] < 0.5) ? (2.0 * dest[i] * src[i] + dest[i] * dest[i] * (1.0 - 2.0 * src[i])) : (sqrt(dest[i]) * (2.0 * src[i] - 1.0) + 2.0 * dest[i] * (1.0 - src[i]));
    return col;
}

//------------------------------------------------------------------------------
/**
*/
vec3
rgb2hsl(vec3 col)
{
    const float eps = 0.0000001;
    float minc = min(col.r, min(col.g, col.b));
    float maxc = max(col.r, max(col.g, col.b));
    vec3 mask = step(col.grr, col.rgb) * step(col.bbg, col.rgb);
    vec3 h = mask * (vec3(0.0, 2.0, 4.0) + (col.gbr - col.brg) / (maxc - minc + eps)) / 6.0;
    return vec3(fract(1.0 + h.x + h.y + h.z),                  // H
                  (maxc - minc) / (1.0 - abs(minc + maxc - 1.0) + eps),   // S
                  (minc + maxc) * 0.5);                            // L
}

//------------------------------------------------------------------------------
/**
*/
vec3
hsl2rgb(vec3 c)
{
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendHue(vec3 src, vec3 dest)
{
    vec3 baseHSL = rgb2hsl(dest);
    return hsl2rgb(vec3(rgb2hsl(src).r, baseHSL.g, baseHSL.b));
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendSaturation(vec3 src, vec3 dest)
{
    vec3 baseHSL = rgb2hsl(dest);
    return hsl2rgb(vec3(baseHSL.r, rgb2hsl(src).g, baseHSL.b));
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendColor(vec3 src, vec3 dest)
{
    vec3 blendHSL = rgb2hsl(src);
    return hsl2rgb(vec3(blendHSL.r, blendHSL.g, rgb2hsl(dest).b));
}

//------------------------------------------------------------------------------
/**
*/
vec3
BlendLuminosity(vec3 src, vec3 dest)
{
    vec3 baseHSL = rgb2hsl(dest);
    return hsl2rgb(vec3(baseHSL.r, baseHSL.g, rgb2hsl(src).b));
}

//------------------------------------------------------------------------------
/**
*/
vec4
CalcBlend(vec2 uv, vec2 objUv, vec4 color, vec4 data0)
{
    const uint BlendOp_Clear = 0u;
    const uint BlendOp_Source = 1u;
    const uint BlendOp_Over = 2u;
    const uint BlendOp_In = 3u;
    const uint BlendOp_Out = 4u;
    const uint BlendOp_Atop = 5u;
    const uint BlendOp_DestOver = 6u;
    const uint BlendOp_DestIn = 7u;
    const uint BlendOp_DestOut = 8u;
    const uint BlendOp_DestAtop = 9u;
    const uint BlendOp_XOR = 10u;
    const uint BlendOp_Darken = 11u;
    const uint BlendOp_Add = 12u;
    const uint BlendOp_Difference = 13u;
    const uint BlendOp_Multiply = 14u;
    const uint BlendOp_Screen = 15u;
    const uint BlendOp_Overlay = 16u;
    const uint BlendOp_Lighten = 17u;
    const uint BlendOp_ColorDodge = 18u;
    const uint BlendOp_ColorBurn = 19u;
    const uint BlendOp_HardLight = 20u;
    const uint BlendOp_SoftLight = 21u;
    const uint BlendOp_Exclusion = 22u;
    const uint BlendOp_Hue = 23u;
    const uint BlendOp_Saturation = 24u;
    const uint BlendOp_Color = 25u;
    const uint BlendOp_Luminosity = 26u;
    
    vec4 src = FillImage(uv, color);
    vec4 dest = sample2D(texture2, UISampler, objUv);
    switch (uint(data0.y + 0.5))
    {
        case BlendOp_Clear:                     return vec4(0.0, 0.0, 0.0, 0.0);
        case BlendOp_Source:                    return src;
        case BlendOp_Over:                      return src + dest * (1.0 - src.a);
        case BlendOp_In:                        return src * dest.a;
        case BlendOp_Out:                       return src * (1.0 - dest.a);
        case BlendOp_Atop:                      return src * dest.a + dest * (1.0 - src.a);
        case BlendOp_DestOver:                  return src * (1.0 - dest.a) + dest;
        case BlendOp_DestIn:                    return dest * src.a;
        case BlendOp_DestOut:                   return dest * (1.0 - src.a);
        case BlendOp_DestAtop:                  return src * (1.0 - dest.a) + dest * src.a;
        case BlendOp_XOR:                       return saturate(src * (1.0 - dest.a) + dest * (1.0 - src.a));
        case BlendOp_Darken:                    return vec4(min(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Add:                       return saturate(src + dest);
        case BlendOp_Difference:                return vec4(abs(dest.rgb - src.rgb) * src.a, dest.a * src.a);
        case BlendOp_Multiply:                  return vec4(src.rgb * dest.rgb * src.a, dest.a * src.a);
        case BlendOp_Screen:                    return vec4((1.0 - ((1.0 - dest.rgb) * (1.0 - src.rgb))) * src.a, dest.a * src.a);
        case BlendOp_Overlay:                   return vec4(BlendOverlay(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Lighten:                   return vec4(max(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_ColorDodge:                return vec4(BlendColorDodge(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_ColorBurn:                 return vec4(BlendColorBurn(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_HardLight:                 return vec4(BlendOverlay(dest.rgb, src.rgb) * src.a, dest.a * src.a);
        case BlendOp_SoftLight:                 return vec4(BlendSoftLight(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Exclusion:                 return vec4((dest.rgb + src.rgb - 2.0 * dest.rgb * src.rgb) * src.a, dest.a * src.a);
        case BlendOp_Hue:                       return vec4(BlendHue(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Saturation:                return vec4(BlendSaturation(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Color:                     return vec4(BlendColor(src.rgb, dest.rgb) * src.a, dest.a * src.a);
        case BlendOp_Luminosity:                return vec4(BlendLuminosity(src.rgb, dest.rgb) * src.a, dest.a * src.a);
    }
    return src;
}

//------------------------------------------------------------------------------
/**
*/
float 
GradientNoise(in vec2 uv)
{
    const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(uv, magic.xy)));
}

//------------------------------------------------------------------------------
/**
*/
float 
Ramp(in float inMin, in float inMax, in float val)
{
    return saturate((val - inMin) / (inMax - inMin));
}

//------------------------------------------------------------------------------
/**
*/
vec4 
Blend(vec4 src, vec4 dest) 
{
    vec4 result;
    result.rgb = src.rgb + dest.rgb * (1.0 - src.a);
    result.a = src.a + dest.a * (1.0 - src.a);
    return result;
}

//------------------------------------------------------------------------------
/**
*/
float 
InnerStroke(float stroke_width, float d) 
{
    return min(Antialias(-d, AA_WIDTH, 0.0), 1.0 - Antialias(-d, AA_WIDTH, stroke_width));
}


//------------------------------------------------------------------------------
/**
*/
vec4 
FillPatternImage(vec2 objUv, vec4 color)
{
    vec4 tile_rect_uv = TileRectUV();
    vec2 tile_size = TileSize();
    vec2 p = objUv;
    // Apply the affine matrix
    vec2 transformed_coords = TransformAffine(p,
      PatternTransformA(), PatternTransformB(), PatternTransformC());
    // Convert back to uv coordinate space
    transformed_coords /= tile_size;
    // Wrap UVs to [0.0, 1.0] so texture repeats properly
    vec2 uv = fract(transformed_coords);
    // Clip to tile-rect UV
    uv *= tile_rect_uv.zw - tile_rect_uv.xy;
    uv += tile_rect_uv.xy;
    return FillImage(uv, color);
}

//------------------------------------------------------------------------------
/**
*/
vec4 
FillRoundedRect(vec2 uv, vec4 color, vec4 data0, vec4 data1, vec4 data2, vec4 data3, vec4 data4) 
{
    vec2 p = uv;
    vec2 size = data0.zw;
    p = (p - 0.5) * size;
    float d = sdRoundRect(p, size, data1, data2);

    // Fill background
    float alpha = Antialias(-d, AA_WIDTH, 0.0);
    vec4 outColor = color * alpha;

    // Draw stroke
    float stroke_width = data3.x;
    vec4 stroke_color = data4;
    if (stroke_width > 0.0) {
        alpha = InnerStroke(stroke_width, d);
        vec4 stroke = stroke_color * alpha;
        outColor = Blend(stroke, outColor);
    }
    return outColor;
}

//------------------------------------------------------------------------------
/**
*/
vec4 
FillBoxShadow(vec2 objUv, vec4 color, vec4 data0, vec4 data1, vec4 data2, vec4 data3, vec4 data4, vec4 data5, vec4 data6)
{
    vec2 p = objUv;
    bool inset = bool(uint(data0.y + 0.5));
    float radius = data0.z;
    vec2 origin = data1.xy;
    vec2 size = data1.zw;
    vec2 clip_origin = data4.xy;
    vec2 clip_size = data4.zw;
    float sdClip = sdRoundRect(p - clip_origin, clip_size, data5, data6);
    float sdRect = sdRoundRect(p - origin, size, data2, data3);
    float clip = inset ? -sdRect : sdClip;
    float d = inset ? -sdClip : sdRect;
    if (clip < 0.0) 
    {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }

    float alpha = radius >= 1.0 ? pow(Antialias(-d, radius * 2 + 0.2, 0.0), 1.9) * 3.3 / pow(radius * 1.2, 0.15) :
        Antialias(-d, AA_WIDTH, inset ? -1.0 : 1.0);
    alpha = clamp(alpha, 0.0, 1.0) * color.a;
    return vec4(color.rgb * alpha, alpha);
}

//------------------------------------------------------------------------------
/**
*/
struct GradientStop 
{ 
    float percent; 
    vec4 color; 
};

//------------------------------------------------------------------------------
/**
*/
GradientStop 
GetGradientStop(uint offset, vec4 data2, vec4 data3, vec4 data4, vec4 data5, vec4 data6)
{
    GradientStop result;
    if (offset < 4u) 
    {
        result.percent = data2[offset];
        if (offset == 0u)
            result.color = data3;
        else if (offset == 1u)
            result.color = data4;
        else if (offset == 2u)
            result.color = data5;
        else if (offset == 3u)
            result.color = data6;
    }
    else 
    {
        result.percent = Scalar(offset - 4u);
        result.color = Vector[offset - 4u];
    }
    return result;
}

//------------------------------------------------------------------------------
/**
*/
vec4 
FillPatternGradient(vec2 uv, vec4 color, vec4 data0, vec4 data1, vec4 data2, vec4 data3, vec4 data4, vec4 data5, vec4 data6)
{
    int num_stops = int(Gradient_NumStops(data0));
    bool is_radial = Gradient_IsRadial(data0);
    vec2 p0 = Gradient_P0(data1);
    vec2 p1 = Gradient_P1(data1);
    float t = 0.0;
    if (is_radial) 
    {
        float r0 = p1.x;
        float r1 = p1.y;
        t = distance(uv, p0);
        float rDelta = r1 - r0;
        t = saturate((t / rDelta) - (r0 / rDelta));
    }
    else 
    {
        vec2 V = p1 - p0;
        t = saturate(dot(uv - p0, V) / dot(V, V));
    }

    vec4 outColor = color;
    GradientStop stop0 = GetGradientStop(0u, data2, data3, data4, data5, data6);
    GradientStop stop1 = GetGradientStop(1u, data2, data3, data4, data5, data6);
    outColor = mix(stop0.color, stop1.color, Ramp(stop0.percent, stop1.percent, t));
    if (num_stops > 2) 
    {
        GradientStop stop2 = GetGradientStop(2u, data2, data3, data4, data5, data6);
        outColor = mix(outColor, stop2.color, Ramp(stop1.percent, stop2.percent, t));
        if (num_stops > 3) 
        {
            GradientStop stop3 = GetGradientStop(3u, data2, data3, data4, data5, data6);
            outColor = mix(outColor, stop3.color, Ramp(stop2.percent, stop3.percent, t));
            if (num_stops > 4) 
            {
                GradientStop stop4 = GetGradientStop(4u, data2, data3, data4, data5, data6);
                outColor = mix(outColor, stop4.color, Ramp(stop3.percent, stop4.percent, t));
                if (num_stops > 5) 
                {
                    GradientStop stop5 = GetGradientStop(5u, data2, data3, data4, data5, data6);
                    outColor = mix(outColor, stop5.color, Ramp(stop4.percent, stop5.percent, t));
                    if (num_stops > 6) 
                    {
                        GradientStop stop6 = GetGradientStop(6u, data2, data3, data4, data5, data6);
                        outColor = mix(outColor, stop6.color, Ramp(stop5.percent, stop6.percent, t));
                    }
                }
            }
        }
    }
    return outColor;
    // Add gradient noise to reduce banding (+4/-4 gradations)
    //out_Color += (8.0/255.0) * gradientNoise(gl_FragCoord.xy) - (4.0/255.0);
}

//------------------------------------------------------------------------------
/**
*/
vec4
FillBlend(vec2 uv, vec2 objUv, vec4 color, vec4 data0)
{
    return CalcBlend(uv, objUv, color, data0);
}

//------------------------------------------------------------------------------
/**
*/
vec4
FillMask(vec2 uv, vec2 objUv, vec4 color) 
{
    vec4 outColor = FillImage(uv, color);
    float alpha = sample2D(texture2, UISampler, objUv).a;
    return outColor * alpha;
}

//------------------------------------------------------------------------------
/**
*/
vec4 
FillGlyph(vec2 uv, vec4 color, vec4 data0) 
{
    float alpha = sample2D(texture1, UISampler, uv).a * color.a;
    float fill_color_luma = data0.y;
    fill_color_luma = pow(alpha, fill_color_luma / 2.2f);
    //float corrected_alpha = alpha;
    return vec4(color.rgb * fill_color_luma, fill_color_luma);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psFill2(
    in vec4 color
    , in vec2 uv
    , in vec2 objUv
    , in vec4 data0
    , in vec4 data1
    , in vec4 data2
    , in vec4 data3
    , in vec4 data4
    , in vec4 data5
    , in vec4 data6
    , [color0] out vec4 Color
)
{
    const uint FillType_Solid = 0u;
    const uint FillType_Image = 1u;
    const uint FillType_Pattern_Image = 2u;
    const uint FillType_Pattern_Gradient = 3u;
    const uint FillType_RESERVED_1 = 4u;
    const uint FillType_RESERVED_2 = 5u;
    const uint FillType_RESERVED_3 = 6u;
    const uint FillType_Rounded_Rect = 7u;
    const uint FillType_Box_Shadow = 8u;
    const uint FillType_Blend = 9u;
    const uint FillType_Mask = 10u;
    const uint FillType_Glyph = 11u;

    vec4 outColor = color;
    switch (FillType(data0))
    {
        case FillType_Solid:                outColor = FillSolid(outColor); break;
        case FillType_Image:                outColor = FillImage(uv, outColor); break;
        case FillType_Pattern_Image:        outColor = FillPatternImage(objUv, outColor); break;
        case FillType_Pattern_Gradient:     outColor = FillPatternGradient(uv, outColor, data0, data1, data2, data3, data4, data5, data6); break;
        case FillType_Rounded_Rect:         outColor = FillRoundedRect(uv, outColor, data0, data1, data2, data3, data4); break;
        case FillType_Box_Shadow:           
        { 
            outColor = FillBoxShadow(objUv, outColor, data0, data1, data2, data3, data4, data5, data6);
            if (outColor.a == 0.0f)
                discard;
            break; 
        }
        case FillType_Blend:                outColor = FillBlend(uv, objUv, outColor, data0); break;
        case FillType_Mask:                 outColor = FillMask(uv, objUv, outColor); break;
        case FillType_Glyph:                outColor = FillGlyph(uv, outColor, data0); break;
    }

    Color = ApplyClip(outColor, objUv);
}

shader
void
vsMain3(
    out vec2 uv
)
{
    vec4 pos_uv = GenerateQuad(gl_VertexID); 
    uv = pos_uv.zw;
    gl_Position = vec4(pos_uv.xy, 0, 1);
}

shader
void 
psFill3(
    in vec2 uv
    , [color0] out vec4 Color
)
{
    Color = sample2D(texture1, UISampler, uv);
}

render_state UltralightState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = One;
    DstBlend[0] = OneMinusSrcAlpha;
    SrcBlendAlpha[0] = OneMinusDstAlpha;
    DstBlendAlpha[0] = One;
    DepthWrite = false;
    DepthEnabled = false;
    CullMode = None;
    ScissorEnabled = true;
};

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Ultralight1, "Ultralight1", vsMain1(), psFill1(), UltralightState);
SimpleTechnique(Ultralight2, "Ultralight2", vsMain2(), psFill2(), UltralightState);
SimpleTechnique(Ultralight3, "Ultralight3", vsMain3(), psFill3(), UltralightState);
