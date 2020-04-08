//------------------------------------------------------------------------------
//  unit.fx
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------

#include <lib/geometrybase.fxh>
#include <lib/techniques.fxh>
#include <lib/colorblending.fxh>
#include <lib/shared.fxh>

group(BATCH_GROUP) shared constant UnitBlock
{
	textureHandle TeamColorMask;
	textureHandle SpecularMap;
	textureHandle RoughnessMap;
	vec4 TeamColor;
};

sampler_state TeamSampler
{
	// Samplers = { TeamColorMask };
	Filter = MinMagMipLinear;
	AddressU = Wrap;
	AddressV = Wrap;
};

//------------------------------------------------------------------------------
/**
	Not a ubershader
*/
[earlydepth]
shader
void
psNotaUnit(
	in vec3 Tangent,
	in vec3 Normal,
	in vec3 Binormal,
	in vec2 UV,
	in vec3 WorldViewVec,
	[color0] out vec4 Albedo,
	[color1] out vec3 Normals,
	[color2] out vec4 Material)
{
	vec4 diffColor = sample2D(AlbedoMap, TeamSampler, UV) * vec4(MatAlbedoIntensity.rgb, 1);
    float roughness = sample2D(RoughnessMap, TeamSampler, UV).r * MatRoughnessIntensity;
	vec4 specColor = sample2D(SpecularMap, TeamSampler, UV) * MatSpecularIntensity;
	float cavity = 1.0f;
    float teamMask = sample2D(TeamColorMask, TeamSampler, UV).r;
	vec4 maskColor = TeamColor * teamMask;

	vec4 normals = sample2D(NormalMap, NormalSampler, UV);
	vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));

	vec4 spec = vec4(specColor.r, roughness, cavity, 0);
	vec4 albedo = diffColor + vec4(Overlay(diffColor.rgb, maskColor.rgb), 0);

	Material = spec;
	Albedo = albedo;
	Normals = bumpNormal;

	// New material setup:
	//vec4 albedo = 		calcColor(sample2D(AlbedoMap, MaterialSampler, UV)) * MatAlbedoIntensity;
	//vec4 material = 	calcMaterial(sample2D(ParameterMap, MaterialSampler, UV));
	//vec4 normals = 		sample2D(NormalMap, NormalSampler, UV);
	//float teamMask = 	sample2D(TeamColorMask, TeamSampler, UV).r;
	//vec4 maskColor = 	TeamColor * teamMask;
	//vec3 bumpNormal = normalize(calcBump(Tangent, Binormal, Normal, normals));
	//Albedo = albedo + vec4(Overlay(albedo.rgb, maskColor.rgb), 0);;
	//Normals = bumpNormal;
	//Material = material;

}

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
	Static,
	"Static",
	vsStatic(),
	psNotaUnit(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	),
	StandardState);

//------------------------------------------------------------------------------
/**
*/
SimpleTechnique(
	Skinned,
	"Skinned",
	vsSkinned(),
	psNotaUnit(
		calcColor = SimpleColor,
		calcBump = NormalMapFunctor,
		calcMaterial = DefaultMaterialFunctor,
		calcDepth = ViewSpaceDepthFunctor,
		calcEnv = PBR
	),
	StencilState);
