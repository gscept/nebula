//------------------------------------------------------------------------------
//  emissive.fx
//  (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"


// material properties
group(BATCH_GROUP) shared constant EmissiveParams[string Visibility = "PS";]
{
    vec4 EmissiveColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
};

render_state EmissiveState
{
    DepthWrite = true;
    DepthEnabled = true; 
    CullMode = None;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
psEmissive(
    in vec3 Tangent,
    in vec3 Normal,
    in flat float Sign,
    in vec2 UV,
    in vec3 WorldSpacePos,
    [color0] out vec4 OutColor
)
{
    OutColor = EmissiveColor;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
    Static,
    "Static",
    vsStatic(),
    psEmissive(),
EmissiveState);

