//------------------------------------------------------------------------------
//  gaussianblur_cs_rgba32f.fx
//
//	Performs a 5x5 gaussian blur as a compute shader, producing a 16 bit floating point texture
//
//  (C) 2016 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D SourceMap;
write rgba32f image2D DestinationMap;
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

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 256
shader
void
csMain() 
{
	const float x = float(gl_GlobalInvocationID.x);
	const float y = float(gl_GlobalInvocationID.y);
	
	vec2 pixelSize = GetPixelSize(SourceMap);
	ivec2 uv = ivec2(x, y);
    vec4 sampleColor = vec4(0.0);
    int i;
    for (i = 0; i < 13; i++)
    {
        sampleColor += sampleOffsetWeights[i].z * texture(SourceMap, (vec2(uv) + sampleOffsetWeights[i].xy) * pixelSize);
    }
    imageStore(DestinationMap, uv, sampleColor);
}

//------------------------------------------------------------------------------
/**
*/
program Blur [ string Mask = "Alt0"; ]
{
	ComputeShader = csMain();
};
