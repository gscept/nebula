//------------------------------------------------------------------------------
//  hbaoblur.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

float PowerExponent = {1.0f};
float BlurFalloff;
float BlurDepthThreshold;

#include "lib/util.fxh"

/// Declaring used textures
Texture2D HBAOBuffer;

/// Declaring used samplers
SamplerState DefaultSampler;


float2 kernel[] = {
	float2(0.5142, 0.6165),
	float2(-0.3512, -0.2231),
	float2(0.4147, 0.1242),
	float2(-0.2526, 0.1662),
	float2(0.3134, -0.2526),
	float2(-0.3725, 0.1724),
	float2(0.1252, 0.38314),
	float2(0.2127, -0.2667),
	float2(-0.1231, 0.4142),
	float2(0.3124, 0.2253),
	float2(-0.5661, -0.7112),
	float2(0.9937, 0.2521),
	float2(0.4132, 0.4566),
	float2(-0.7264, -0.1778),
	float2(0.3831, 0.8636),
	float2(-0.3573, -0.7284),
};

BlendState Blend 
{
};

// we use the same rasterizer for both states
RasterizerState Rasterizer
{
	CullMode = 2;
};

DepthStencilState DepthStencil
{
	DepthWriteMask = 0;
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
void
vsMain(float4 position : POSITION,
	float2 uv0 : TEXCOORD0,
	out float4 Position : SV_POSITION0,
	out float2 UV : TEXCOORD0) 
{
	Position = position;
	UV = uv0;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 Position : SV_POSITION0,
	in float2 screenUv : TEXCOORD0,
	out float4 Color : SV_TARGET0) 
{
	float2 pixelSize = GetPixelSize(HBAOBuffer);
	
	float sample = 0;
	float2 ao = HBAOBuffer.Sample(DefaultSampler, screenUv);
	float ao_total = ao.x;
	float center_d = ao.y;
	float w_total = 1;
	
	int i;
	for (i = 0; i < 13; i++)
	{
		float r = 2.0f*i + (-BLUR_RADIUS+0.5f);
		float2 value = HBAOBuffer.Sample(DefaultSampler, screenUv + kernel[i].xy * pixelSize.xy);
		float w = CrossBilateralWeight(r, value.y, center_d);
		ao_total += w * value.x;
		w_total += w;
	}
	Color = pow(ao_total/w_total, PowerExponent);
}

//------------------------------------------------------------------------------
/**
*/
VertexShader vs = CompileShader(vs_5_0, vsMain());
PixelShader ps = CompileShader(ps_5_0, psMain());

technique11 Default
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(vs);
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(ps);
	}
}