//------------------------------------------------------------------------------
//  grid.fx
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"
#include "lib/techniques.fxh"

float GridSize = 50;
textureHandle GridTex;
mat4x4 PlaneProjection;

sampler_state GridSampler
{
	//Samplers = { GridTex };
	MaxAnisotropic = 16;
	Filter = Anisotropic;
};

render_state GridState
{
	BlendEnabled[0] = true;
	SrcBlend[0] = SrcAlpha;
	DstBlend[0] = OneMinusSrcAlpha;
	CullMode = None;
	DepthClamp = false;
	DepthWrite = false;
	PolygonOffsetEnabled = true;
	PolygonOffsetFactor = 1.0f;
	PolygonOffsetUnits = 2.0f;
};

//------------------------------------------------------------------------------
/**
*/
shader
void
vsGrid(
	[slot=0] in vec2 position,
	out vec4 WorldPos) 
{
	vec4 pos = vec4(position.x + EyePos.x, 0, position.y + EyePos.z, 1);
	gl_Position = Projection * View * pos;
	WorldPos = pos;
}

//------------------------------------------------------------------------------
/**
*/
shader
void
psGrid(
	in vec4 WorldPos, 
	[color0] out vec4 color)
{
	float len = 1.0 - smoothstep(1.0f, 250.0f, distance(EyePos.xz, WorldPos.xz));
	vec2 uv = (WorldPos.xz / vec2(GridSize)) - vec2(0.5);
	vec4 c = texture(sampler2D(Textures2D[GridTex], GridSampler), uv).rgba;
	//vec2 line = (vec2(cos(WorldPos.x / GridSize), cos(WorldPos.z / GridSize)) - vec2(0.90, 0.90));
	//float c = saturate(max(line.x, line.y)) * 5;
	
	//c = smoothstep(0.5f, 1, c);
	float alpha = len * c.a;
	color = vec4(c.rgb, saturate(alpha));
}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(Default, "Static", vsGrid(), psGrid(), GridState);
