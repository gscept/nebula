//------------------------------------------------------------------------------
//  horizontalbloom.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

float HDRBloomScale = {1.f};

#include "lib/util.fxh"

/// Declaring used textures
Texture2D SourceTexture;

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


// bloom samples
static const int MaxBloomSamples = 16;

//------------------------------------------------------------------------------
/**
*/
void
vsMain(float4 position : POSITION,
	float2 uv0 : TEXCOORD0,
	out float2 UV : TEXCOORD0,
	out float4 Position : SV_POSITION0) 
{
	Position = position;
	UV = uv0;
}

//------------------------------------------------------------------------------
/**
    Calculate a gaussian distribution
*/    
float
GaussianDistribution(in const float x, in const float myu, in const float rho)
{
    const float sqrt2pi = 2.5066283f;
    float g = 1.0f / (sqrt2pi * rho);
    g *= exp(-(x * x + myu * myu) / (2 * rho * rho));
    return g;
}

//------------------------------------------------------------------------------
/**
    UpdateSamplesBloom
    Get sample offsets and weights for a horizontal or vertical bloom filter.
    This is normally executed in the pre-shader.
*/
void
UpdateSamplesBloom(in bool horizontal, in float pixelSize, in float deviation, in float multiplier, out float3 sampleOffsetsWeights[MaxBloomSamples])
{    
    // fill center texel
    float weight = multiplier * GaussianDistribution(0.0f, 0.0f, deviation);
    sampleOffsetsWeights[0]  = float3(0.0f, 0.0f, weight);
    sampleOffsetsWeights[15] = float3(0.0f, 0.0f, weight);
	
    // fill first half
    int i;
    for (i = 1; i < 8; i++)
    {
        if (horizontal)
        {
            sampleOffsetsWeights[i].xy = float2(i * pixelSize, 0.0f);
        }
        else
        {
            sampleOffsetsWeights[i].xy = float2(0.0f, i * pixelSize);
        }
        weight = multiplier * GaussianDistribution((float)i, 0, deviation);
        sampleOffsetsWeights[i].z = weight;
    }
	
    // mirror second half
    for (i = 8; i < 15; i++)
    {
        sampleOffsetsWeights[i] = sampleOffsetsWeights[i - 7] * float3(-1.0f, -1.0f, 1.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
psMain(in float2 UV : TEXCOORD0,
	in float4 Position : SV_POSITION0,
	out float4 BloomedColor : SV_TARGET0) 
{
	float2 PixelSize = GetPixelSize(SourceTexture);
    float3 sampleOffsetsWeights[MaxBloomSamples];
    UpdateSamplesBloom(true, PixelSize.x, 3.0f, HDRBloomScale, sampleOffsetsWeights);
    
    int i;
    float4 color = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (i = 0; i < MaxBloomSamples; i++)
    {
        color += sampleOffsetsWeights[i].z * DecodeHDR(SourceTexture.Sample(DefaultSampler, UV + sampleOffsetsWeights[i].xy));
    }
    BloomedColor = EncodeHDR(color);
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