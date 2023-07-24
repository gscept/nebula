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
vsEmissive(
    [slot=0] in vec3 position,
    [slot=2] in vec2 uv,
    out vec2 UV
    )
{
    vec4 modelSpace = Model * vec4(position, 1);
    gl_Position = ViewProjection * modelSpace;
    UV          = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psEmissive(
    in vec2 uv,
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
    vsEmissive(),
    psEmissive(),
EmissiveState);

