//------------------------------------------------------------------------------
//  subsurface.fxh
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef SUBSURFACE_FXH
#define SUBSURFACE_FXH

#include "lib/std.fxh"
#include "lib/util.fxh"

/// Declaring used samplers
samplerstate SubsurfaceSampler
{
	Samplers = { Mask, LightTexture, DepthTexture, AbsorptionTexture, ScatterTexture };
	//AddressU = Mirror;
	//AddressV = Mirror;
	Filter = Point;
};

sampler2D LightTexture;
sampler2D DepthTexture;
sampler2D AbsorptionTexture;
sampler2D ScatterTexture;
sampler2D EmissiveTexture;
sampler2D Mask;

const float SubsurfaceStrength  = 1.45f;
const float SubsurfaceWidth = 0.20f;
const float SubsurfaceCorrection = 50.0f;

#ifdef VERTICAL
vec2 SubsurfaceDirection = vec2(1, 0);
#else
vec2 SubsurfaceDirection = vec2(0, 1);
#endif

const vec4 kernel[] = {
    vec4(0.530605, 0.613514, 0.739601, 0),
    vec4(0.0402784, 0.0657244, 0.04631, -0.0833333),
    vec4(0.0211412, 0.0459286, 0.0378196, -0.0208333),
    vec4(0.0211412, 0.0459286, 0.0378196, 0.0208333),
    vec4(0.0402784, 0.0657244, 0.04631, 0.0833333),
    vec4(0.0493588, 0.0367726, 0.0219485, 0.1875),
    vec4(0.0410172, 0.0199899, 0.0118481, 0.333333),
    vec4(0.0263642, 0.0119715, 0.00684598, 0.520833),
    vec4(0.017924, 0.00711691, 0.00347194, 0.75),
    vec4(0.017924, 0.00711691, 0.00347194, -0.75),
    vec4(0.0263642, 0.0119715, 0.00684598, -0.520833),
    vec4(0.0410172, 0.0199899, 0.0118481, -0.333333),
    vec4(0.0493588, 0.0367726, 0.0219485, -0.1875),
    vec4(0.000973794, 1.11862e-005, 9.43437e-007, -3),
    vec4(0.00333804, 7.85443e-005, 1.2945e-005, -2.52083),
    vec4(0.00500364, 0.00020094, 5.28848e-005, -2.08333),
    vec4(0.00700976, 0.00049366, 0.000151938, -1.6875),
    vec4(0.0094389, 0.00139119, 0.000416598, -1.33333),
    vec4(0.0128496, 0.00356329, 0.00132016, -1.02083),    
    vec4(0.0128496, 0.00356329, 0.00132016, 1.02083),
    vec4(0.0094389, 0.00139119, 0.000416598, 1.33333),
    vec4(0.00700976, 0.00049366, 0.000151938, 1.6875),
    vec4(0.00500364, 0.00020094, 5.28848e-005, 2.08333),
    vec4(0.00333804, 7.85443e-005, 1.2945e-005, 2.52083),
    vec4(0.000973794, 1.11862e-005, 9.43437e-007, 3)
};

state SubsurfaceState
{
	CullMode = Back;
	DepthEnabled = true;
	DepthWrite = false;
	DepthFunc = Always;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = FlipY(uv);
}

//------------------------------------------------------------------------------
/**
	Calculates subsurface approximation based on screen-space criteria.
	Should only be applied to skins!
*/
shader
void
psMain(in vec2 UV,
	[color0] out vec4 result)
{
	vec4 mask = textureLod(Mask, UV, 0);
	
	// clip pixel if the mask is 0
	if (mask.a == 0) { discard; return; }
	
	float w[6] = { 0.006,   0.061,   0.242,  0.242,  0.061, 0.006 };
	float o[6] = {  -1.0, -0.6667, -0.3333, 0.3333, 0.6667,   1.0 };

	// sample data textures
	vec4 absorption = textureLod(AbsorptionTexture, UV, 0);
	vec4 scatter = textureLod(ScatterTexture, UV, 0);
	vec4 emissive = textureLod(EmissiveTexture, UV, 0);
	
	// get color and depth
	vec4 light = DecodeHDR(textureLod(LightTexture, UV, 0));
	vec3 color = (light.rgb + emissive.rgb) * scatter.rgb;
	
	float depth = textureLod(DepthTexture, UV, 0).r;
	
	// create blurred color
	vec3 colorBlurred = color;
	//colorBlurred *= 0.382f;
	
	// calculate final step
	vec2 pixelSize = GetPixelSize(LightTexture);
	vec2 finalStep = (exp2(5 * mask.r + 1) * pixelSize * SubsurfaceDirection) / depth;
	
	// get other samples
	for (int i = 0; i < 6; i++)
	{
		vec2 offset = UV + o[i] * finalStep * mask.g;
		vec3 lightSample = DecodeHDR(textureLod(LightTexture, offset, 0)).rgb;
		float depthSample = textureLod(DepthTexture, offset, 0).r;
		
		float s = min(0.0125 * exp2(5 * mask.b + 1) * abs(depth - depthSample), 1.0f);
		lightSample = lerp(lightSample, color, s);
		
		// accumulate
		colorBlurred += w[i] * lightSample;
	}
	
	// multiply by absorption
	colorBlurred *= absorption.rgb;
	
	// finally return color
	result = EncodeHDR(vec4(colorBlurred, 1));
}

#endif