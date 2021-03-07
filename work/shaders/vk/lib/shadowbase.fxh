//------------------------------------------------------------------------------
//  shadowbase.fxh
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------


#ifndef SHADOWBASE_FXH
#define SHADOWBASE_FXH

#include "lib/defaultsamplers.fxh"

const float DepthScaling = 5.0f;
const float DarkeningFactor = 1.0f;
const float ShadowConstant = 100.0f;

sampler_state ShadowSampler
{
    //Samplers = { AlbedoMap, DisplacementMap };
};

render_state ShadowState
{
    CullMode = Back;
    DepthClamp = false;
    DepthEnabled = false;
    DepthWrite = false;
    BlendEnabled[0] = true;
    SrcBlend[0] = One;
    DstBlend[0] = One;
    BlendOp[0] = Min;
};

render_state ShadowStateCSM
{
    CullMode = Back;
    DepthClamp = false;
    DepthEnabled = false;
    DepthWrite = false;
    BlendEnabled[0] = true;
    SrcBlend[0] = One;
    DstBlend[0] = One;
    BlendOp[0] = Min;
};

render_state ShadowStateMSM
{
    CullMode = Back;
    DepthClamp = false;
    DepthEnabled = false;
    DepthWrite = false;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    ProjPos = LightViewMatrix[gl_InstanceID] * Model * vec4(position, 1);
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinned(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    [slot=7] in vec4 weights,
    [slot=8] in uvec4 indices,
    out vec2 UV,
    out vec4 ProjPos)
{
    vec4 skinnedPos = SkinnedPosition(position, weights, indices);
    ProjPos = LightViewMatrix[gl_InstanceID] * Model * skinnedPos;
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInst(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    int viewStride = gl_InstanceID % 16;
    ProjPos = LightViewMatrix[viewStride] * ModelArray[gl_InstanceID] * vec4(position, 1);;
    gl_Position = ProjPos;
    gl_Layer = viewStride;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticCSM(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    ProjPos = CSMViewMatrix[gl_InstanceID] * Model * vec4(position, 1);
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinnedCSM(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    [slot=7] in vec4 weights,
    [slot=8] in uvec4 indices,
    out vec2 UV,
    out vec4 ProjPos)
{
    vec4 skinnedPos = SkinnedPosition(position, weights, indices);
    ProjPos = CSMViewMatrix[gl_InstanceID] * Model * skinnedPos;
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInstCSM(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    int viewStride = gl_InstanceID % 4;
    ProjPos = CSMViewMatrix[viewStride] * ModelArray[gl_InstanceID] * vec4(position, 1);
    gl_Position = ProjPos;
    gl_Layer = viewStride;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticPoint(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    ProjPos = LightViewMatrix[gl_InstanceID] * Model * vec4(position, 1);
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinnedPoint(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    [slot=7] in vec4 weights,
    [slot=8] in uvec4 indices,
    out vec2 UV,
    out vec4 ProjPos)
{
    vec4 skinnedPos = SkinnedPosition(position, weights, indices);
    ProjPos = LightViewMatrix[gl_InstanceID] * Model * skinnedPos;
    gl_Position = ProjPos;
    gl_Layer = gl_InstanceID;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInstPoint(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 ProjPos)
{
    int viewStride = gl_InstanceID % 6;
    ProjPos = LightViewMatrix[viewStride] * ModelArray[gl_InstanceID] * vec4(position, 1);
    gl_Position = ProjPos;
    gl_Layer = viewStride;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psShadow(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psShadowAlpha(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
    if (alpha < AlphaSensitivity) discard;

    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psESM(in vec2 UV,
      in vec4 ProjPos,
      [color0] out float ShadowColor)
{
    ShadowColor = (ProjPos.z/ProjPos.w) * DepthScaling;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psESMAlpha(in vec2 UV,
      in vec4 ProjPos,
      [color0] out float ShadowColor)
{
    float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
    if (alpha < AlphaSensitivity) discard;
    ShadowColor = (ProjPos.z/ProjPos.w) * DepthScaling;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psVSM(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    //float dx = dFdx(depth);
    //float dy = dFdy(depth);
    //moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psVSMAlpha(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
    if (alpha < AlphaSensitivity) discard;

    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    //float dx = dFdx(depth);
    //float dy = dFdy(depth);
    //moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psMSM(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec4 Moments)
{
    float depth = ProjPos.z;
    Moments = EncodeMSM4(depth);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMSMAlpha(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec4 Moments)
{
    float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
    if (alpha < AlphaSensitivity) discard;

    float depth = ProjPos.z;
    Moments = EncodeMSM4(depth);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psVSMPoint(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    //float dx = dFdx(depth);
    //float dy = dFdy(depth);
    //moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psVSMAlphaPoint(in vec2 UV,
    in vec4 ProjPos,
    [color0] out vec2 ShadowColor)
{
    float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
    if (alpha < AlphaSensitivity) discard;

    float depth = ProjPos.z / ProjPos.w;
    float moment1 = depth;
    float moment2 = depth * depth;

    // Adjusting moments (this is sort of bias per pixel) using derivative
    //float dx = dFdx(depth);
    //float dy = dFdy(depth);
    //moment2 += 0.25f*(dx*dx+dy*dy);

    ShadowColor = vec2(moment1, moment2);
}

//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
Variance(vec2 shadowSample,
         float lightSpaceDepth,
         float tolerance)
{
    // get average and average squared
    float avgZ = shadowSample.x;
    float avgZ2 = shadowSample.y;

    // assume that if the projected depth is less than the average in the pixel, the pixel must be lit
    if (lightSpaceDepth <= avgZ)
    {
        return 1.0f;
    }
    else
    {
        float variance 	= (avgZ2) - (avgZ * avgZ);
        variance 		= min(1.0f, max(0.0f, variance + tolerance));

        float mean 		= avgZ;
        float d			= lightSpaceDepth - mean;
        float p_max		= variance / (variance + d*d);

        // to avoid light bleeding, change this constant
        return max(p_max, float(lightSpaceDepth <= avgZ));
    }
}

//------------------------------------------------------------------------------
/**
    Calculates Chebyshevs upper bound for use with VSM shadow mapping with local lights
*/
float
ChebyshevUpperBound(vec2 Moments, float t, float tolerance)
{
    // One-tailed inequality valid if t > Moments.x
    if (t <= Moments.x) return 1.0f;

    // Compute variance.
    float Variance = Moments.y - (Moments.x*Moments.x);
    Variance = max(Variance, tolerance);

    // Compute probabilistic upper bound.
    float d = t - Moments.x;
    float p_max = Variance / (Variance + d*d);

    return linstep(0.95f, 1.0f, p_max);
}

//------------------------------------------------------------------------------
/**
*/
float
ExponentialShadowSample(float mapDepth, float depth, float bias)
{
    float receiverDepth = DepthScaling * depth - bias;
    float occluderReceiverDistance = mapDepth - receiverDepth;
    float occlusion = saturate(exp(DarkeningFactor * occluderReceiverDistance));
    //float occlusion = saturate(exp(DarkeningFactor * occluderReceiverDistance));
    return occlusion;
}

//------------------------------------------------------------------------------
/**
*/
const mat4 EncodeMSMMatrix = mat4(-2.07224649f, 13.7948857237f, 0.105877704f, 9.7924062118f,
                     32.23703778f, -59.4683975703f, -1.9077466311f, -33.7652110555f,
                    -68.571074599f, 82.0359750338f, 9.3496555107f, 47.9456096605f,
                     39.3703274134f, -35.364903257f, -6.6543490743f, -23.9728048165f);

const mat4 DecodeMSMMatrix = mat4(0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
                 0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
                 0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
                 0.0319417555f, -0.1722823173f, -0.2758014811f, -0.3376131734f);
//------------------------------------------------------------------------------
/**
*/
vec4
EncodeMSM4(float depth)
{
    float sq = depth * depth;
    vec4 moments = vec4(depth, sq, sq * depth, sq * sq);
    moments = EncodeMSMMatrix * moments;
    moments[0] += 0.035955884801f;
    return moments;
}

//------------------------------------------------------------------------------
/**
*/
vec4
EncodeMSM4Vec(vec4 moments)
{
    vec4 res = moments;
    res = EncodeMSMMatrix * res;
    res[0] += 0.035955884801f;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
vec4
DecodeMSM4(vec4 moments)
{
    vec4 res = moments;
    res[0] -= 0.035955884801f;
    res = DecodeMSMMatrix * res;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
float
MSMShadowSample(vec4 moments, float momentBias, float depth, float depthBias)
{
    vec4 decoded = DecodeMSM4(moments);
    vec4 b = lerp(decoded, vec4(0.5f), momentBias);
    vec3 z;
    z[0] = depth - depthBias;

    float l32d22 = mad(-b[0], b[1], b[2]);
    float d22 = mad(-b[0], b[0], b[1]);
    float squaredVariance = mad(-b[1], b[1], b[3]);
    float d33d22 = dot(vec2(squaredVariance, -l32d22), vec2(d22, l32d22));
    float invd22 = 1.0f / d22;
    float l32 = l32d22 * invd22;

    vec3 c = vec3(1, z[0], z[0] * z[0]);
    c[1] -= b.x;
    c[2] -= b.y + l32 * c[1];
    c[1] *= invd22;
    c[2] *= d22 / d33d22;
    c[1] -= l32 * c[2];
    c[0] -= dot(c.yz, b.xy);
    float p = c[1] / c[2];
    float q = c[0] / c[2];
    float r = sqrt((p * p * 0.25f) - q);
    z[1] = -p * 0.5f - r;
    z[2] = -p * 0.5f + r;
    vec4 select =
        (z[2] < z[0]) ? vec4(z[1], z[0], 1, 1) : (
        (z[1] < z[0]) ? vec4(z[0], z[1], 0, 1) :
        vec4(0));
    float quot = (select[0] * z[2] - b[0] * (select[0] + z[2]) + b[1]) / ((z[2] - select[1]) * (z[0] - z[1]));
    return 1.0f - (select[2] + select[3] * quot);
}


#endif // SHADOWBASE_FXH
