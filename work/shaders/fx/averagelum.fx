//------------------------------------------------------------------------------
//  averagelum.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"

Texture2D ColorSource;
Texture2D PreviousLum;
SamplerState DefaultSampler;

float4 Luminance = {0.299f, 0.587f, 0.114f, 0.0f};
float TimeDiff;

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
	DepthEnable = FALSE;
	DepthWriteMask = 0;
};


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
	Performs a 2x2 kernel downscale
*/
void
psMain(float4 Position : SV_POSITION0,
	float2 UV : TEXCOORD0,
	out float result : SV_TARGET0)
{
	float2 pixelSize = GetPixelSize(ColorSource);
	float fAvg = 0.0f;
	
	// source should be a 512x512 texture, so we sample the 8'th mip of the texture
	float sample1 = dot(ColorSource.SampleLevel(DefaultSampler, UV + float2(0.5f, 0.5f) * pixelSize, 8), Luminance);
	float sample2 = dot(ColorSource.SampleLevel(DefaultSampler, UV + float2(0.5f, -0.5f) * pixelSize, 8), Luminance);
	float sample3 = dot(ColorSource.SampleLevel(DefaultSampler, UV + float2(-0.5f, 0.5f) * pixelSize, 8), Luminance);
	float sample4 = dot(ColorSource.SampleLevel(DefaultSampler, UV + float2(-0.5f, -0.5f) * pixelSize, 8), Luminance);
	fAvg = (sample1+sample2+sample3+sample4) * 0.25f;
	
	float fAdaptedLum = PreviousLum.Sample(DefaultSampler, float2(0.5f, 0.5f));
	result = clamp(fAvg + (fAdaptedLum - fAvg) * ( 1 - pow( 0.98f, TimeDiff * 60)), 0.3f, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
technique11 Default
{
	pass Main
	{
		SetBlendState(Blend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF);
		SetDepthStencilState(DepthStencil, 0);
		SetRasterizerState(Rasterizer);
		SetVertexShader(CompileShader(vs_5_0, vsMain()));
		SetHullShader(NULL);
		SetDomainShader(NULL);
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, psMain()));
	}
}