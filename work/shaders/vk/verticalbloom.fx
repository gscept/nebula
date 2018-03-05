//------------------------------------------------------------------------------
//  horizontalbloom.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float HDRBloomScale = 1.f;

sampler2D SourceTexture;

samplerstate BloomSampler
{
	Samplers = { SourceTexture };
	Filter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

state BloomState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

// bloom samples
#define MAXBLOOMSAMPLES 16

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
	UV = uv;
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
UpdateSamplesBloom(in bool horizontal, in float pixelSize, in float deviation, in float multiplier, out vec3 sampleOffsetsWeights[MAXBLOOMSAMPLES])
{    
    // fill center texel
    float weight = multiplier * GaussianDistribution(0.0f, 0.0f, deviation);
    sampleOffsetsWeights[0]  = vec3(0.0f, 0.0f, weight);
    sampleOffsetsWeights[15] = vec3(0.0f, 0.0f, weight);
	
    // fill first half
    int i;
    for (i = 1; i < 8; i++)
    {
        if (horizontal)
        {
            sampleOffsetsWeights[i].xy = vec2(i * pixelSize, 0.0f);
        }
        else
        {
            sampleOffsetsWeights[i].xy = vec2(0.0f, i * pixelSize);
        }
        weight = multiplier * GaussianDistribution(float(i), 0, deviation);
        sampleOffsetsWeights[i].z = weight;
    }
	
    // mirror second half
    for (i = 8; i < 15; i++)
    {
        sampleOffsetsWeights[i] = sampleOffsetsWeights[i - 7] * vec3(-1.0f, -1.0f, 1.0f);
    }
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
	[color0] out vec4 BloomedColor) 
{
	vec2 pixelSize = GetPixelSize(SourceTexture);
    vec3 sampleOffsetsWeights[MAXBLOOMSAMPLES];
    UpdateSamplesBloom(false, pixelSize.x, 3.0f, HDRBloomScale, sampleOffsetsWeights);
    
    int i;
    vec4 color = vec4( 0.0f, 0.0f, 0.0f, 0.0f );
    for (i = 0; i < MAXBLOOMSAMPLES; i++)
    {
        color += sampleOffsetsWeights[i].z * textureLod(SourceTexture, UV + sampleOffsetsWeights[i].xy, 0);
    }
    BloomedColor = color;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), BloomState);