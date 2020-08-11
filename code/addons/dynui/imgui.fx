//------------------------------------------------------------------------------
//  imgui.fx
//
//	Shader for ImGUI rendering
//
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/techniques.fxh" 
#include "lib/shared.fxh"
#include "lib/defaultsamplers.fxh"

// put variables in push-constant block
push constant ImGUI [ string Visibility = "PS|VS"; ]
{
	mat4 TextProjectionModel;
	uint PackedTextureInfo;
};

group(BATCH_GROUP) sampler_state TextureSampler
{
	Filter = Point;
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
void UnpackTexture(uint val, out uint id, out uint type, out uint mip, out uint layer, out uint useAlpha)
{
	type = val & 0xF;
	layer = (val >> 4) & 0xFF;
	mip = (val >> 12) & 0xFF;
	useAlpha = (val >> 20) & 0x1;
	id = (val >> 21) & 0xFFF;
}

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
	gl_Position = ImGUI.TextProjectionModel * pos;
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
	vec4 texColor;
	uint id, type, layer, mip, useAlpha;
	UnpackTexture(ImGUI.PackedTextureInfo, id, type, mip, layer, useAlpha);
	if (type == 0)
		texColor = sample2DLod(id, TextureSampler, UV, mip);
	else if (type == 1)
		texColor = sample2DArrayLod(id, TextureSampler, vec3(UV, layer), mip);
	else if (type == 2)
	{
		ivec3 size = textureSize(make_sampler3D(id, TextureSampler), int(mip));
		texColor = sample3DLod(id, TextureSampler, vec3(UV, layer / float(size.z)), mip);
	}

	if (useAlpha == 0)
		texColor.a = 1;
	FinalColor = Color * texColor;
}


//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsMain(), psMain(), TextState);
