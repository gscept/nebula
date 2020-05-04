//------------------------------------------------------------------------------
//  terrain.fx
//  (C) 2020 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/techniques.fxh"
#include "lib/defaultsamplers.fxh"
#include "lib/shadowbase.fxh"
#include "lib/geometrybase.fxh"

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) constant TerrainUniforms [ string Visibility = "VS"; ]
{
	mat4 Transform;
};

//------------------------------------------------------------------------------
/**
	Simple terrain vertex shader
*/
shader
void
vsTerrain(
	[slot=0] in vec3 position,
	[slot=1] in vec3 normal,
	[slot=2] in vec2 uv,
	out vec3 Normal,
	out vec2 UV,
	out vec3 WorldViewVec) 
{
	vec4 modelSpace = Transform * vec4(position, 1);
	gl_Position = ViewProjection * modelSpace;
	UV = uv;
	
	Normal = (Transform * vec4(normal, 0)).xyz;
	WorldViewVec = modelSpace.xyz - EyePos.xyz;
}


//------------------------------------------------------------------------------
/**
	Pixel shader for multilayered painting
*/
shader
void
psTerrain(
	in vec3 Normal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material) 
{	
	Material = vec4(0.5f, 0.5f, 0.5f, 0.5f);
	Albedo = vec4(1,1,1,1);
	Normals = Normal;
}

//------------------------------------------------------------------------------
/**
*/
render_state TerrainState
{
	CullMode = Back;
};

SimpleTechnique(Terrain, "Terrain", vsTerrain(), psTerrain(), TerrainState);
SimpleTechnique(TerrainSunShadows, "Terrain Sun Shadows", vsStaticCSM(), psShadow(), ShadowStateCSM);
SimpleTechnique(TerrainLocalShadows, "Terrain Local Shadows", vsStatic(), psShadow(), ShadowState);