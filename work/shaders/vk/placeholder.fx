//------------------------------------------------------------------------------
//  placeholder.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"
#include "lib/objects_shared.fxh"

textureHandle AlbedoMap;

//------------------------------------------------------------------------------
/**
*/
shader
void
vsMain(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
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
psMain(in vec2 uv,
	[color0] out vec4 Color)
{
	Color = sample2D(AlbedoMap, Basic2DSampler, uv);
}

//------------------------------------------------------------------------------
/**
*/
StateLessTechnique(Static, "Static", vsMain(), psMain());

// add a skinned variation since the Character system automatically appends the Skinned feature string when rendering characters
StateLessTechnique(Skinned, "Skinned", vsMain(), psMain());
