//------------------------------------------------------------------------------
//  text.fx
//
//	Basic text shader
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh"
#include "lib/shared.fxh"

group(BATCH_GROUP) texture2D Texture;

/// Declaring used textures
group(BATCH_GROUP) push constant Text
{
	mat4 TextProjectionModel;
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
	gl_Position = Text.TextProjectionModel * pos;
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
	vec4 texColor = texture(sampler2D(Texture, TextureSampler), UV).rrrr;
	FinalColor = texColor * Color;
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), TextState);