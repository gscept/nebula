//------------------------------------------------------------------------------
//  pbr.fxh
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#ifndef PBR_FXH
#define PBR_FXH
#include "lib/std.fxh"
#include "lib/shared.fxh"
#include "lib/util.fxh"

const float RimIntensity = 0.9;//3.0;//
const float RimPower = 2.0;

// Definitions for the current setup of the material buffer
#define MAT_METALLIC 0
#define MAT_ROUGHNESS 1
#define MAT_CAVITY 2
#define MAT_EMISSIVE 3

//------------------------------------------------------------------------------
/**
    Compute a rim light intensity value.
*/
float RimLightIntensity(vec3 worldNormal,     // surface normal in world space
                        vec3 worldEyeVec)     // eye vector in world space
{
    float rimIntensity  = pow(abs(1.0f - abs(dot(worldNormal, worldEyeVec))), RimPower);
    rimIntensity *= RimIntensity;
    return rimIntensity;
}

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
	float denom = NdotV * (1.0f - k) + k;
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

void
CalculateF0(in vec3 color, in float metallic, inout vec3 F0)
{
    // F0 as 0.04 will usually look good for all dielectric (non-metal) surfaces
	//F0 = vec3(0.04);
	// for metallic surfaces we interpolate between F0 and the albedo value with metallic value as our lerp weight
	F0 = mix(F0, color.rgb, metallic);
}

//------------------------------------------------------------------------------
/**
*/

void
CalculateBRDF(
    float NdotH,
    float NdotL,
    float NdotV,
    float cosTheta, // HdotL for direct lighting, NdotV for IBL. Theta is angle of incidence
    float roughness,
    vec3 F0,
    out vec3 F, // fresnel term
    out vec3 brdf)
{
    // if we are using IBL we should use dot(N,V) for cosTheta. 
    // The correct way of doing it for direct lighting is using the halfway H for each lightsource (HdotL)
    F = FresnelSchlickGloss(F0, max(cosTheta, 0.0), roughness);

    float NDF = NormalDistributionGGX(NdotH, roughness);
    float G = GeometrySmith(NdotV, NdotL, roughness);
    
    // Calculate Cook-Torrance BRDF
    vec3 nominator = NDF * G * F;
    float denominator = 4 * NdotV * NdotL + 0.001f; //We add 0.001f in case dot ends up becoming zero.
    brdf = nominator / denominator;
}

//------------------------------------------------------------------------------
/**
	Calculates light diffuse and specular using physically based lighting
*/
void
BRDFLighting(
	 float NH,
	 float NL,
	 float NV,
	 float HL,
	 float specPower,
	 vec3 specColor,
	 out vec3 spec)
{
	float normalizationTerm = (specPower + 2.0f) / 8.0f;
	float blinnPhong = pow(NH, specPower);
	float specularTerm = blinnPhong;
	float cosineTerm = NL;
	vec3 fresnelTerm = FresnelSchlick(specColor, HL);
	float alpha = 1.0f / ( sqrt ( PI_OVER_FOUR * specPower + PI_OVER_TWO) );
	float visibilityTerm = (NL * (1.0f - alpha) + alpha ) * (NV * ( 1.0f - alpha ) + alpha );
	visibilityTerm = 1.0f / visibilityTerm;
	spec = specularTerm * cosineTerm * fresnelTerm * visibilityTerm;
	//spec = fresnelTerm * (roughness + 2) / 8.0f * pow(NH, roughness) * (NL);
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
