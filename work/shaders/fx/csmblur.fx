//------------------------------------------------------------------------------
//  gaussianblur.fx
//
//	Performs a 5x5 gaussian blur as a post effect
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"

Texture2D SourceMap;

SamplerState GaussianSampler
{
	Filter = MIN_MAG_MIP_POINT;
};

static const float3 sampleOffsetWeights[] = {
    float3( -1.5,  0.5, 0.024882 ),
    float3( -0.5, -0.5, 0.067638 ),
    float3( -0.5,  0.5, 0.111515 ),
    float3( -0.5,  1.5, 0.067638 ),
    float3(  0.5, -1.5, 0.024882 ),
    float3(  0.5, -0.5, 0.111515 ),
    float3(  0.5,  0.5, 0.183858 ),
    float3(  0.5,  1.5, 0.111515 ),
    float3(  0.5,  2.5, 0.024882 ),
    float3(  1.5, -0.5, 0.067638 ),
    float3(  1.5,  0.5, 0.111515 ),
    float3(  1.5,  1.5, 0.067638 ),
    float3(  2.5,  0.5, 0.024882 )
};

BlendState Blend 
{
};

RasterizerState Rasterizer
{
	CullMode = 1;
	DepthClipEnable = FALSE;
};

DepthStencilState DepthStencil
{
	DepthEnable = 0;
	DepthWriteMask = 0;
};

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float3 position : POSITION,
	float2 uv : TEXCOORD0,
	out float4 Position : SV_POSITION,
	out float2 UV : TEXCOORD1) 
{
	Position = float4(position, 1);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float4 position : SV_POSITION,
	in float2 uv : TEXCOORD1,
	out float2 Color : SV_TARGET0) 
{
	float2 pixelSize = GetPixelSize(SourceMap);
    float2 sampleColor = 0;
    int i;
    for (i = 0; i < 13; i++)
    {
		float2 uvSample = uv + sampleOffsetWeights[i].xy * pixelSize.xy;
        sampleColor += sampleOffsetWeights[i].z * SourceMap.Sample(GaussianSampler, uvSample).rg;
    }
    Color = sampleColor;
}

//------------------------------------------------------------------------------
/**
*/

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
