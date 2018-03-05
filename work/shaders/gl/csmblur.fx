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

// border intervals are used to determine which shadowmap inside the atlas we are currently treating
//vec2 BorderIntervals;

samplerstate GaussianSampler
{
	Samplers = { SourceMap };
	AddressU = Border;
	AddressV = Border;
	Filter = Point;
	BorderColor = {0, 0, 0, 0};
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
	[color0] out vec2 Color) 
{
	// calculate limits
	vec2 pixelSize = GetPixelSize(SourceMap);
	//vec2 sampleColor = vec2(0,0);
    vec2 sampleColor;
	/*if (sampleColor.r == -1)
	{
		Color = vec2(0,0);
		return;
	}*/
	
    int i;
    for (i = 0; i < 4; i++)
    {
		vec2 uvSample = uv + sampleOffsetWeights[i].xy * pixelSize.xy;
		vec2 currentSample = texture(SourceMap, uvSample).rg;		
        sampleColor += currentSample;
    }
    Color = sampleColor * 0.25f;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), GaussianBlurState);
