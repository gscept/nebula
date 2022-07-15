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
	[color0] out float ShadowColor)
{
	ShadowColor = ProjPos.z / ProjPos.w;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psShadowAlpha(in vec2 UV,
	in vec4 ProjPos,
	[color0] out float ShadowColor)
{
	float alpha = sample2D(AlbedoMap, ShadowSampler, UV).a;
	if (alpha < AlphaSensitivity) 
        discard;
    ShadowColor = ProjPos.z / ProjPos.w;
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

	return p_max;
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
float
PCFShadow(textureHandle shadowMap, vec2 uv, vec2 texelSize)
{
    float shadow = 0.0f;
    vec3 offsets = vec3(-2, 2, 0) * texelSize.xyx;
    
    float samp0 = sample2DLod(shadowMap, ShadowSampler, uv + offsets.xz, 0).r;
    float samp1 = sample2DLod(shadowMap, ShadowSampler, uv + offsets.yz, 0).r;
    float samp2 = sample2DLod(shadowMap, ShadowSampler, uv + offsets.zx, 0).r;
    float samp3 = sample2DLod(shadowMap, ShadowSampler, uv + offsets.zy, 0).r;
    float samp4 = sample2DLod(shadowMap, ShadowSampler, uv, 0).r;
    return (samp0 + samp1 + samp2 + samp3 + samp4) * 0.2f;
}
#endif // SHADOWBASE_FXH
