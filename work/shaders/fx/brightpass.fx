//------------------------------------------------------------------------------
//  brightpass.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"
#include "lib/shared.fxh"

cbuffer HDRVars
{
	float HDRBrightPassThreshold = {1.0f};
	float4 HDRBloomColor = {1.0f, 1.0f, 1.0f, 1.0f};
	float4 Luminance = {0.299f, 0.587f, 0.114f, 0.0f};
}


/// Declaring used textures
Texture2D ColorSource;
Texture2D LuminanceTexture;

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
	float4 sample = DecodeHDR( ColorSource.Sample( DefaultSampler, UV ) );
	
	// Get the calculated average luminance 
	float fLumAvg = LuminanceTexture.Sample(DefaultSampler, float2(0.5f, 0.5f)).r;     
	
	sample = ToneMap(sample, fLumAvg, Luminance);
	float3 brightColor = max(sample.rgb - HDRBrightPassThreshold, 0);
	Color = HDRBloomColor * float4(brightColor, sample.a);
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