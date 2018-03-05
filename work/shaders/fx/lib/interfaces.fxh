#ifndef INTERFACES_FXH
#define INTERFACES_FXH

//------------------------
/**
	Shader designer interfaces and classes library file
*/
#include "../../lib/util.fxh"
#include "../../lib/defaultsampler.fxh"

Texture2D DarkMap;

//------------------------
/**
	Interface for lighting an object
*/
interface Lighting
{
	float4 SampleLight(float4 diffColor, float4 emissiveColor, float4 specColor, float4 lightValues, float2 UV, float emissiveIntensity, float specularIntensity, float brightness);
};

//-----------------------
/**
	Shading the light using a lightmap as modifier
*/
class Lightmapped : Lighting
{	
	float4 SampleLight(float4 diffColor, float4 emissiveColor, float4 specColor, float4 lightValues, float2 UV, float emissiveIntensity, float specularIntensity, float brightness)
	{
		float4 lightMapColor = DarkMap.Sample(lightMapSampler, UV);
		diffColor = diffColor * lightMapColor * brightness;
		
		return psLightMaterial(lightValues, diffColor, emissiveColor, emissiveIntensity, specColor, specularIntensity);
	}
};

//-----------------------
/**
	Shading the light using a lightmap as modifier
*/
class Plain : Lighting
{
	float4 SampleLight(float4 diffColor, float4 emissiveColor, float4 specColor, float4 lightValues, float2 UV, float emissiveIntensity, float specularIntensity, float brightness)
	{
		return psLightMaterial(lightValues, diffColor, emissiveColor, emissiveIntensity, specColor, specularIntensity);
	}
};

// end of Lighting interface


#endif