//------------------------------------------------------------------------------
//  gaussianblur.fx
//
//	Performs a 5x5 gaussian blur as a post effect
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/util.fxh"

/// Declaring used textures
Texture2D SourceMap;

/// Declaring used samplers
SamplerState DefaultSampler;


float3 sampleOffsetWeights[13] = {
    { -1.5,  0.5, 0.024882 },
    { -0.5, -0.5, 0.067638 },
    { -0.5,  0.5, 0.111515 },
    { -0.5,  1.5, 0.067638 },
    {  0.5, -1.5, 0.024882 },
    {  0.5, -0.5, 0.111515 },
    {  0.5,  0.5, 0.183858 },
    {  0.5,  1.5, 0.111515 },
    {  0.5,  2.5, 0.024882 },
    {  1.5, -0.5, 0.067638 },
    {  1.5,  0.5, 0.111515 },
    {  1.5,  1.5, 0.067638 },
    {  2.5,  0.5, 0.024882 },
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
	in float2 ScreenUv : TEXCOORD0,
	out float4 Color : SV_TARGET0) 
{

	float2 pixelSize = GetPixelSize(SourceMap);
    float4 sample = float4(0.0, 0.0, 0.0, 0.0);
    int i;
    for (i = 0; i < 13; i++)
    {
        sample += sampleOffsetWeights[i].z * SourceMap.Sample(DefaultSampler, ScreenUv + sampleOffsetWeights[i].xy * pixelSize.xy);
    }
    Color = sample;
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