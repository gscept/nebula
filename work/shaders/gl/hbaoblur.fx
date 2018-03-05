//------------------------------------------------------------------------------
//  hbaoblur.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float PowerExponent = 1.0f;
float BlurFalloff;
float BlurDepthThreshold;

sampler2D HBAOBuffer;

samplerstate HBAOSampler
{
	Samplers = { HBAOBuffer };
	Filter = Point;
	AddressU = Border;
	AddressV = Border;
	BorderColor = { 0,0,0,0 };
};


const vec2 kernel[] = {
	vec2(0.5142, 0.6165),
	vec2(-0.3512, -0.2231),
	vec2(0.4147, 0.1242),
	vec2(-0.2526, 0.1662),
	vec2(0.3134, -0.2526),
	vec2(-0.3725, 0.1724),
	vec2(0.1252, 0.38314),
	vec2(0.2127, -0.2667),
	vec2(-0.1231, 0.4142),
	vec2(0.3124, 0.2253),
	vec2(-0.5661, -0.7112),
	vec2(0.9937, 0.2521),
	vec2(0.4132, 0.4566),
	vec2(-0.7264, -0.1778),
	vec2(0.3831, 0.8636),
	vec2(-0.3573, -0.7284)
};

state HBAOBlurState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

#define BLUR_RADIUS 16
#define HALF_BLUR_RADIUS (BLUR_RADIUS/2)

//----------------------------------------------------------------------------------
float CrossBilateralWeight(float r, float d, float d0)
{
    // The exp2(-r*r*g_BlurFalloff) expression below is pre-computed by fxc.
    // On GF100, this ||d-d0||<threshold test is significantly faster than an exp weight.
    return exp2(-r*r*BlurFalloff) * float(abs(d - d0) < BlurDepthThreshold);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(in vec3 position,
	[slot=2] in vec2 uv,
	out vec2 UV) 
{
	gl_Position = vec4(position, 1);
	UV = FlipY(uv);
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 uv,
	[color0] out vec4 Color) 
{
	float2 pixelSize = GetPixelSize(HBAOBuffer);
	float sampleColor = 0;

	vec2 ao = textureLod(HBAOBuffer, uv, 0).rg;
    float ao_total = ao.x;
	float center_d = ao.y;
	float w_total = 1;
	int i;		
	for (i = 0; i < BLUR_RADIUS; i++)
	{
		// Sample the pre-filtered data with step size = 2 pixels
		float r = 2.0f*i + (-BLUR_RADIUS+0.5f);
		vec2 value = textureLod(HBAOBuffer, uv + kernel[i].xy * pixelSize.xy * 2.5f, 0).rg;
		float w = CrossBilateralWeight(r, value.y, center_d);
		ao_total += w * value.x;
		w_total += w;
	}

	Color = vec4(pow(ao_total/w_total, PowerExponent));
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), HBAOBlurState);