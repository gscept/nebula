//------------------------------------------------------------------------------
//  gltf.fx
//  (C) 2020 Individual contributors, See LICENSE file
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/geometrybase.fxh"
#include "lib/techniques.fxh"

group(BATCH_GROUP) shared constant GLTFBlock
{
	// lower camel case names by design, just to keep it consistent with the GLTF standard.
	textureHandle baseColorTexture;
	textureHandle normalTexture;
	textureHandle metallicRoughnessTexture;
	vec4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
};

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
	
	vec4 normals = sample2D(normalTexture, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	Albedo = baseColor;
	Normals = bumpNormal;
	Material[MAT_METALLIC] = material.b;
	Material[MAT_ROUGHNESS] = material.g;
	Material[MAT_CAVITY] = 1; // TODO: implement sampler for cavity
	Material[MAT_EMISSIVE] = 0; // TODO: implement sampler for emissive
}

//------------------------------------------------------------------------------
//	Standard methods
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
	GLTFStaticEnvironment, 
	"Static|Environment", 
	vsStatic(), 
	psGLTF(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor
	),
	StandardState);
