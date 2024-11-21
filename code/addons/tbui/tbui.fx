//------------------------------------------------------------------------------
//  tbui.fx
//
//  Shader for Turbobadger rendering
//
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh" 
#include "lib/shared.fxh"
#include "lib/defaultsamplers.fxh"

// put variables in push-constant block
push constant TBUI [ string Visibility = "PS|VS"; ]
{
    mat4 TextProjectionModel;
    uint Texture;
};

group(BATCH_GROUP) sampler_state TextureSampler
{
    Filter = Linear;
};

render_state TextState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    DepthWrite = false;
    DepthEnabled = false;
    CullMode = None;
    ScissorEnabled = true;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec2 position,
    [slot=1] in vec2 uv,
    [slot=2] in vec4 color, 
    out vec2 UV,
    out vec4 Color) 
{
    vec4 pos = vec4(position, 0, 1);    
    gl_Position = TBUI.TextProjectionModel * pos;
    Color = color;
    UV = uv;
}

//------------------------------------------------------------------------------
/** 
*/
shader
void
psMain(
    in vec2 UV,
    in vec4 Color,
    [color0] out vec4 FinalColor) 
{
    vec4 texColor = sample2D(TBUI.Texture, TextureSampler, UV);

    // Since we are using sRGB output, remember to degamma
    FinalColor = Color * texColor;
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), TextState);
