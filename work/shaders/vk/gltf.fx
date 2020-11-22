//------------------------------------------------------------------------------
//  gltf.fx
//  (C) 2020 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"

render_state DoubleSidedState
{
	CullMode = None;
};

group(BATCH_GROUP) shared constant GLTFBlock
{
	// lower camel case names by design, just to keep it consistent with the GLTF standard.
	textureHandle baseColorTexture;
	textureHandle normalTexture;
	textureHandle metallicRoughnessTexture;
	textureHandle emissiveTexture;
	textureHandle occlusionTexture;
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	float metallicFactor;
	float roughnessFactor;
	float alphaCutoff;
};

float Greyscale(in vec4 color)
{
	return dot(color.rgb, vec3(0.299, 0.587, 0.114));
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psGLTF(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material)
{
	vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV) * vec4(baseColorFactor.rgb, 1);
	vec4 material = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
	vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
	vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Albedo = baseColor;
	Normals = bumpNormal;
	Material[MAT_METALLIC] = material.b;
	Material[MAT_ROUGHNESS] = material.g;
	Material[MAT_CAVITY] = Greyscale(occlusion);
	Material[MAT_EMISSIVE] = Greyscale(emissive);
}

//------------------------------------------------------------------------------
/**
*/
[earlydepth]
shader
void
psGLTFAlphaMask(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material)
{
	vec4 baseColor = sample2D(baseColorTexture, MaterialSampler, UV) * vec4(baseColorFactor.rgb, 1);

	if(baseColor.a <= alphaCutoff)
		discard;

	vec4 material = sample2D(metallicRoughnessTexture, MaterialSampler, UV) * vec4(1.0f, roughnessFactor, metallicFactor, 1.0f);
	vec4 emissive = sample2D(emissiveTexture, MaterialSampler, UV) * emissiveFactor;
	vec4 occlusion = sample2D(occlusionTexture, MaterialSampler, UV);
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Albedo = baseColor;
	Normals = bumpNormal;
	Material[MAT_METALLIC] = material.b;
	Material[MAT_ROUGHNESS] = material.g;
	Material[MAT_CAVITY] = Greyscale(occlusion);
	Material[MAT_EMISSIVE] = Greyscale(emissive);
}

//------------------------------------------------------------------------------
//	Techniques
//------------------------------------------------------------------------------
SimpleTechnique(
	GLTFStatic,
	"Static",
	vsStatic(), 
	psGLTF(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	StandardState);

SimpleTechnique(
	GLTFStaticDoubleSided,
	"Static|DoubleSided",
	vsStatic(), 
	psGLTF(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	DoubleSidedState);

SimpleTechnique(
	GLTFStaticAlphaMask, 
	"Static|AlphaMask", 
	vsStatic(),
	psGLTFAlphaMask(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	StandardState);
	
SimpleTechnique(
	GLTFStaticAlphaMaskDoubleSided, 
	"Static|AlphaMask|DoubleSided", 
	vsStatic(),
	psGLTFAlphaMask(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	DoubleSidedState);
	
SimpleTechnique(
	GLTFStaticAlphaBlend, 
	"Static|AlphaBlend", 
	vsStatic(),
	psGLTF(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	AlphaState);

SimpleTechnique(
	GLTFStaticAlphaBlendDoubleSided, 
	"Static|AlphaBlend|DoubleSided", 
	vsStatic(),
	psGLTF(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	DoubleSidedState);