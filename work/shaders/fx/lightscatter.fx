//------------------------------------------------------------------------------
//  lightscatter.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/util.fxh"
float2 LightPos = float2(0.5f, 0.5f);
float Density = 0.99f;
float Decay = 0.97f;
float Weight = 0.4f;
float Exposure = 0.21f;

#define NUM_SAMPLES 128

/// Declaring used textures
Texture2D ColorSource;

/// Declaring used samplers
SamplerState DefaultSampler;

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
	in float2 UV : TEXCOORD0,
	out float4 Color : SV_TARGET0) 
{
	float2 pixelSize = GetPixelSize(ColorSource);
	float2 screenUV = psComputeScreenCoord(Position.xy, pixelSize.xy);
	float2 deltaTexCoord = float2(screenUV - LightPos);
	deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;
	float3 color = DecodeHDR(ColorSource.Sample(DefaultSampler, screenUV));
	float illuminationDecay = 1.0f;
	
	[unroll]
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		screenUV -= deltaTexCoord;
		float3 sample = DecodeHDR(ColorSource.Sample(DefaultSampler, screenUV));
		sample *= illuminationDecay * Weight;
		color += sample;
		illuminationDecay *= Decay;
	}
	color *= Exposure;
	
	Color = EncodeHDR(float4(color, 1));
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