//------------------------------------------------------------------------------
//  gaussianblur.fx
//
//	Performs a 5x5 gaussian blur as a post effect
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D SourceMap;

samplerstate GaussianSampler
{
	Samplers = { SourceMap };
	Filter = Point;
	AddressU = Clamp;
	AddressV = Clamp;
};

const vec3 sampleOffsetWeights[] = {
    vec3( -1.5,  0.5, 0.024882 ),
    vec3( -0.5, -0.5, 0.067638 ),
    vec3( -0.5,  0.5, 0.111515 ),
    vec3( -0.5,  1.5, 0.067638 ),
    vec3(  0.5, -1.5, 0.024882 ),
    vec3(  0.5, -0.5, 0.111515 ),
    vec3(  0.5,  0.5, 0.183858 ),
    vec3(  0.5,  1.5, 0.111515 ),
    vec3(  0.5,  2.5, 0.024882 ),
    vec3(  1.5, -0.5, 0.067638 ),
    vec3(  1.5,  0.5, 0.111515 ),
    vec3(  1.5,  1.5, 0.067638 ),
    vec3(  2.5,  0.5, 0.024882 )
};

state GaussianBlurState
{
	CullMode = Back;
	DepthEnabled = false;
	DepthWrite = false;
};

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
	vec2 pixelSize = GetPixelSize(SourceMap);
    vec4 sampleColor = vec4(0.0);
    int i;
    for (i = 0; i < 13; i++)
    {
        sampleColor += sampleOffsetWeights[i].z * textureLod(SourceMap, uv + sampleOffsetWeights[i].xy * pixelSize.xy, 0);
    }
    Color = sampleColor;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), GaussianBlurState);
