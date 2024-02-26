//------------------------------------------------------------------------------
//  unlit.fx
//  (C) 2012 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/objects_shared.fxh>
#include <lib/util.fxh>
#include <lib/techniques.fxh>
#include <lib/materialparams.fxh>
#include <lib/defaultsamplers.fxh>

float Brightness = 0.0f;

render_state UnlitOpaqueState
{
    CullMode = Back;
};

render_state UnlitAlphaState
{
    BlendEnabled[0] = true;
    SrcBlend[0] = SrcAlpha;
    DstBlend[0] = OneMinusSrcAlpha;
    CullMode = Back;
};


//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
    [slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec3 tangent,
    [slot=4] in vec3 binormal,
    out vec2 UV) 
{
    gl_Position = ViewProjection * Model * vec4(position, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMain(in vec2 UV,
    [color0] out vec4 Albedo) 
{
    vec4 diffColor = sample2D(AlbedoMap, GeometryTextureSampler, UV.xy);
    float alpha = diffColor.a;
    if (alpha < AlphaSensitivity) discard;
    Albedo = diffColor * Brightness;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psMainAlpha(in vec2 UV,
    [color0] out vec4 Albedo) 
{
    vec4 diffColor = sample2D(AlbedoMap, GeometryTextureSampler, UV.xy);
    float alpha = diffColor.a;
    if (alpha < AlphaSensitivity) discard;
    Albedo = diffColor * AlphaBlendFactor * Brightness;
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsMain(), psMain(), UnlitOpaqueState);
SimpleTechnique(Alpha, "Alpha", vsMain(), psMainAlpha(), UnlitAlphaState);