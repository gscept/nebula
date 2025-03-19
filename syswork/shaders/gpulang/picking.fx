//------------------------------------------------------------------------------
//  picking.fx
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include <lib/std.fxh>
#include <lib/shared.fxh>
#include <lib/skinning.fxh>
#include <lib/techniques.fxh>

#include <material_interfaces.fx>

render_state PickingState
{
    CullMode = Back;
};

render_state BillboardPickingState
{
    CullMode = None;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(
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
vsBillboard(
    [slot=0] in vec2 position,
    [slot=2] in vec2 uv,
    out vec2 UV) 
{
    gl_Position = ViewProjection * Model * vec4(position, 0, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsSkinned(
    [slot=0] in vec3 position,
    [slot=1] in vec4 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec4 tangent,
    [slot=4] in vec4 binormal,
    [slot=7] in vec4 weights,
    [slot=8] in uvec4 indices,
    out vec2 UV) 
{
    vec4 skinnedPos = SkinnedPosition(position, weights, indices);
    gl_Position = ViewProjection * Model * skinnedPos;
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStaticInst(
    [slot=0] in vec3 position,
    [slot=1] in vec3 normal,
    [slot=2] in vec2 uv,
    [slot=3] in vec3 tangent,
    [slot=4] in vec3 binormal,
    out vec2 UV) 
{
    gl_Position = ViewProjection * ModelArray[gl_InstanceIndex] * vec4(position, 1);
    UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
[early_depth]
shader
void
psStatic([color0] out float Id) 
{
    Id = float(ObjectId);
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Static, "Static", vsStatic(), psStatic(), PickingState);
SimpleTechnique(Billboard, "Alt0", vsBillboard(), psStatic(), BillboardPickingState);
SimpleTechnique(Skinned, "Skinned", vsSkinned(), psStatic(), PickingState);
SimpleTechnique(StaticInstanced, "Static|Instanced", vsStaticInst(), psStatic(), PickingState);