//------------------------------------------------------------------------------
//  downsample.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"

Texture2D ColorSource;
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
	out float4 result : SV_TARGET0)
{
	float2 pixelSize = float2(512, 512);
	float4 sample1 = ColorSource.Sample(DefaultSampler, UV + float2(0.5f, 0.5f) * pixelSize);
	float4 sample2 = ColorSource.Sample(DefaultSampler, UV + float2(0.5f, -0.5f) * pixelSize);
	float4 sample3 = ColorSource.Sample(DefaultSampler, UV + float2(-0.5f, 0.5f) * pixelSize);
	float4 sample4 = ColorSource.Sample(DefaultSampler, UV + float2(-0.5f, -0.5f) * pixelSize);
	result = (sample1+sample2+sample3+sample4) * 0.25f;
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