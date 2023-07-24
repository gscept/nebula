//------------------------------------------------------------------------------
//  gui.fx
//
//  Basic GUI shader for use with LibRocket
//
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

/// Declaring used textures
group(BATCH_GROUP) push constant GUI
{
    textureHandle Texture;
    mat4 Transform;
};
sampler_state TextureSampler
{
    //Samplers = { Texture };
};



render_state DefaultGUIState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    CullMode = Back;
    DepthEnabled = false;
};

render_state ScissorGUIState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    CullMode = Back;
    ScissorEnabled = true;
    DepthEnabled = false;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec2 position,
    [slot=1] in vec4 color,
    [slot=2] in vec2 uv,
    out vec2 UV,
    out vec4 Color) 
{
    gl_Position = GUI.Transform * vec4(position, 1, 1);
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
    vec4 texColor = sample2D(GUI.Texture, TextureSampler, UV);
    FinalColor = texColor * Color;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), DefaultGUIState);
SimpleTechnique(Scissor, "Static|Alt0", vsMain(), psMain(), ScissorGUIState);