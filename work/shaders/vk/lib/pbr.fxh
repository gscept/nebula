//------------------------------------------------------------------------------
//  pbr.fxh
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef PBR_FXH
#define PBR_FXH
#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"

// Definitions for the current setup of the material buffer
#define MAT_METALLIC 0
#define MAT_ROUGHNESS 1
#define MAT_CAVITY 2
#define MAT_EMISSIVE 3

//------------------------------------------------------------------------------
/**
	Calculate fresnel amount using Schlick's approximation
*/
vec3
FresnelSchlick(vec3 spec, float dotprod)
{
	float base = 1.0 - clamp(dotprod, 0.0f, 1.0f);
	float exponent = pow(base, 5);
	return spec + (1 - spec) * exponent;
}

//------------------------------------------------------------------------------
/**
	Calculate fresnel amount using Schlick's approximation but with roughness
*/
vec3
FresnelSchlickGloss(vec3 spec, float dotprod, float roughness)
{
	float base = 1.0 - clamp(dotprod, 0.0f, 1.0f);
	float exponent = pow(base, 5);
	return spec + (max(vec3(1 - roughness), spec) - spec) * exponent;
}

//------------------------------------------------------------------------------
/**
    GGX normal distribution function (NDF)
    This approximates the ratio of microfacets aligned to given halfway vector H
*/
float
NormalDistributionGGX(in float NdotH, in float roughness)
{
	float a = pow(roughness, 4);
	float NdotH2 = NdotH * NdotH;
	
	float denom = (NdotH2 * (a - 1.0f) + 1.0f);
	denom = PI * denom * denom;
	
	return (a / (denom + 0.00001f)); // epsilon to avoid division by zero
}

//------------------------------------------------------------------------------
/**
    Smith with Schlick-GGX geometry function
    This statistically approximates the ratio of microfacets that overshadow each other
*/
float
GeometrySchlickGGX(in float NdotV, in float k)
{
	float denom = mad(NdotV, 1.0f - k,  k);
	return NdotV / denom;
}

//------------------------------------------------------------------------------
/**
*/
float
GeometrySmith(in float NdotV, in float NdotL, in float roughness)
{	
	float r = roughness + 1.0f;
	float k = (r*r) * 0.125f; // (r^2 / 8)
	float ggxNV = GeometrySchlickGGX(NdotV, k);
	float ggxNL = GeometrySchlickGGX(NdotL, k);
	return ggxNV * ggxNL;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateF0(in vec3 color, in float metallic, const vec3 def)
{
    // F0 as 0.04 will usually look good for all dielectric (non-metal) surfaces
	// F0 = vec3(0.04);
	// for metallic surfaces we interpolate between F0 and the albedo value with metallic value as our lerp weight
	return mix(def, color.rgb, metallic);
}

//------------------------------------------------------------------------------
/**
	TODO: Add more variants of diffuse lobes.
*/
vec3
DiffuseLobe(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float LdotH)
{
	// Lambert
	return diffuseColor / PI;
}

//------------------------------------------------------------------------------
/**
*/
vec3
SpecularLobe(vec4 material, vec3 F0, vec3 H, float NdotV, float NdotL, float NdotH, float LdotH)
{
	float D = NormalDistributionGGX(NdotH, material[MAT_ROUGHNESS]);
	float G = GeometrySmith(NdotV, NdotL, material[MAT_ROUGHNESS]);
	vec3 F = FresnelSchlickGloss(F0, max(LdotH, 0.0), material[MAT_ROUGHNESS]);
	return (D * G) * F;
}

//------------------------------------------------------------------------------
/**
*/
vec3 EvaluateBRDF(vec3 diffuseColor, vec4 material, vec3 F0, vec3 H, float NdotV, float NdotL, float NdotH, float LdotH)
{
	//material[MAT_ROUGHNESS] = max(0.07f, material[MAT_ROUGHNESS]);
	vec3 diffuseContrib = (diffuseColor * (1.0f - material[MAT_METALLIC]));
	vec3 diffuseTerm = DiffuseLobe(diffuseContrib, material[MAT_ROUGHNESS], NdotV, NdotL, LdotH);
	vec3 specularTerm = SpecularLobe(material, F0, H, NdotV, NdotL, NdotH, LdotH);
	vec3 brdf = diffuseTerm + specularTerm;
	return brdf;
}

//------------------------------------------------------------------------------
/**
	OSM = Occlusion, Smoothness, Metalness
*/
vec4
ConvertOSM(in vec4 material)
{
	vec4 ret;
	ret[MAT_METALLIC] = material.b;
	ret[MAT_ROUGHNESS] = 1 - material.g;
	ret[MAT_CAVITY] = 1 - material.r;
	ret[MAT_EMISSIVE] = material.a;
	return ret;
}

//------------------------------------------------------------------------------
#endif
