//------------------------------------------------------------------------------
//  copy.fx
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

sampler2D CopyBuffer;
sampler_state CopySampler
{
    Samplers = { CopyBuffer };
    Filter = Linear;
};

render_state CopyState
{
    CullMode = None;
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
*/
shader
void
psMain(in vec2 uv,
    [color0] out vec4 Color)
{
    Color = textureLod(CopyBuffer, uv, 0);
}

//------------------------------------------------------------------------------
/**
*/
PostEffect(vsMain(), psMain(), CopyState);
