//------------------------------------------------------------------------------
//  downsample.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D ColorSource;

sampler_state ColorSampler
{
    Samplers = { ColorSource };
    Filter = Point;
};

render_state DownscaleState
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
vsMain(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV) 
{
    gl_Position = vec4(position, 1);
    UV = uv;
    UV.y = 1 - UV.y;
}

//------------------------------------------------------------------------------
/**
    Performs a 2x2 kernel downscale
*/
shader
void
psMain(in vec2 uv,
    [color0] out vec4 result)
{
    vec2 pixelSize = GetPixelSize(ColorSource);
    vec4 sample1 = textureLod(ColorSource, uv + vec2(0.5f, 0.5f) * pixelSize, 0);
    vec4 sample2 = textureLod(ColorSource, uv + vec2(0.5f, -0.5f) * pixelSize, 0);
    vec4 sample3 = textureLod(ColorSource, uv + vec2(-0.5f, 0.5f) * pixelSize, 0);
    vec4 sample4 = textureLod(ColorSource, uv + vec2(-0.5f, -0.5f) * pixelSize, 0);
    result = (sample1+sample2+sample3+sample4) * 0.25f;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), DownscaleState);
