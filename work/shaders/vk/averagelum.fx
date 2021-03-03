//------------------------------------------------------------------------------
//  averagelum.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"

texture2D ColorSource;
texture2D PreviousLum;

constant AverageLumBlock
{
    float TimeDiff;
};


sampler_state LuminanceSampler
{
    //Samplers = { ColorSource, PreviousLum };
    Filter = Point;
};



render_state AverageLumState
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
}

//------------------------------------------------------------------------------
/**
    Performs a 2x2 kernel downscale, will only render 1 pixel
*/
shader
void
psMain(in vec2 UV,
    [color0] out float result)
{
    vec4 sample1 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(1, 0), 0);
    vec4 sample2 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(0, 1), 0);
    vec4 sample3 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(1, 1), 0);
    vec4 sample4 = texelFetch(sampler2D(ColorSource, LuminanceSampler), ivec2(0, 0), 0);
    float currentLum = dot((sample1 + sample2 + sample3 + sample4) * 0.25f, Luminance);
    float lastLum = texelFetch(sampler2D(PreviousLum, LuminanceSampler), ivec2(0, 0), 0).r;

    float lum = lastLum + (currentLum - lastLum) * (1.0 - pow(0.98, 30.0 * TimeDiff));
    lum = max(0.25f, lum);
    result = lum;
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), AverageLumState);
