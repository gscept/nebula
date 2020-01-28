//------------------------------------------------------------------------------
//  lightmapbase.fxh
//  (C) 2015 Gustav Sterbrant
//------------------------------------------------------------------------------

#ifndef LIGHTMAPBASE_FXH
#define LIGHTMAPBASE_FXH
#include "materialparams.fxh"
#include "geometrybase.fxh"

/// Declaring used textures
textureHandle LightMap;
float LightMapIntensity = 0.0f;

samplerstate LightmapSampler
{
	//Samplers = { ParameterMap, EmissiveMap, NormalMap, LightMap, AlbedoMap };
};

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped lit geometry
*/
shader
void
psLightmappedLit(
	in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV1,
	in vec2 UV2,
	[color0] out vec4 Albedo,
	[color1] out vec4 Normals,
	[color2] out float Depth,
	[color3] out vec4 Material) 
{
	vec4 albedo = 		calcColor(sample2D(AlbedoMap, MaterialSampler, UV1)) * MatAlbedoIntensity;
	if (albedo.a < AlphaSensitivity) discard;
	vec4 material = 	calcMaterial(sample2D(ParameterMap, MaterialSampler, UV1));
	vec4 normals = 		sample2D(NormalMap, NormalSampler, UV1);
	vec4 lightMapColor = vec4(((sample2D(LightMap, LightmapSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Depth = length(ViewSpacePos.xyz);
	Albedo = albedo * lightMapColor;
	Normals = PackViewSpaceNormal(bumpNormal);
	Material = material;
}

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped lit geometry together with vertex colors
*/
shader
void
psLightmappedLitVertexColors(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV1,
	in vec2 UV2,
	in vec4 Color,
	[color0] out vec4 Albedo,
	[color1] out vec4 Normals,
	[color2] out float Depth,
	[color3] out vec4 Material) 
{
	vec4 albedo = 		calcColor(sample2D(AlbedoMap, MaterialSampler, UV1)) * MatAlbedoIntensity;
	if (albedo.a < AlphaSensitivity) discard;
	vec4 material = 	calcMaterial(sample2D(ParameterMap, MaterialSampler, UV1));
	vec4 normals = 		sample2D(NormalMap, NormalSampler, UV1);
	vec4 lightMapColor = vec4(((sample2D(LightMap, LightmapSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Depth = length(ViewSpacePos.xyz);
	Albedo = albedo * Color * lightMapColor;
	Normals = PackViewSpaceNormal(bumpNormal);
	Material = material;
}

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped unlit geometry
*/
shader
void
psLightmappedUnlit(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV1,
	in vec2 UV2,
	[color0] out vec4 Albedo) 
{
	vec4 diffColor = sample2D(AlbedoMap, LightmapSampler, UV1.xy);
	vec4 lightMapColor = vec4(((sample2D(LightMap, LightmapSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	Albedo = diffColor * lightMapColor;
	
	float alpha = diffColor.a;
	if (alpha < AlphaSensitivity) discard;
}

//------------------------------------------------------------------------------
/**
	Pixel shader for lightmapped unlit geometry with vertex colors
*/
shader
void
psLightmappedUnlitVertexColors(in vec3 ViewSpacePos,
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV1,
	in vec2 UV2,
	in vec4 Color,
	[color0] out vec4 Albedo) 
{
	vec4 diffColor = sample2D(AlbedoMap, LightmapSampler, UV1.xy);
	vec4 lightMapColor = vec4(((sample2D(LightMap, LightmapSampler, UV2.xy) - 0.5f) * 2.0f * LightMapIntensity).rgb, 1);
	
	Albedo = diffColor * lightMapColor * Color;

	float alpha = diffColor.a;
	if (alpha < AlphaSensitivity) discard;
}

#endif