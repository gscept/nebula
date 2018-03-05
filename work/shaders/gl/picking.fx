//------------------------------------------------------------------------------
//  picking.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/skinning.fxh"
#include "lib/techniques.fxh"

state PickingState
{
	CullMode = Back;
};

state BillboardPickingState
{
	CullMode = None;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsStatic(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
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
vsBillboard(in vec2 position,
	in vec2 uv,
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
vsSkinned(in vec3 position,
	in vec4 normal,
	in vec2 uv,
	in vec4 tangent,
	in vec4 binormal,
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
vsStaticInst(in vec3 position,
	in vec3 normal,
	in vec2 uv,
	in vec3 tangent,
	in vec3 binormal,
	out vec2 UV) 
{
	gl_Position = ViewProjection * ModelArray[gl_InstanceID] * vec4(position, 1);
	UV = uv;
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
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