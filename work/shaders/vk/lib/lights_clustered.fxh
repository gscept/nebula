
//------------------------------------------------------------------------------
//  lights_clustered.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shadowbase.fxh"
#include "lib/pbr.fxh"

struct SpotLight
{
	vec4 position;				// view space position of light, w is range
	vec4 forward;				// forward vector of light (spotlight and arealights)

	vec2 angleSinCos;			// angle cutoffs

	vec3 color;					// light color
	int projectionExtension;	// projection extension index
	int shadowExtension;		// projection extension index
	uint flags;					// feature flags (shadows, projection texture, etc)
};

struct SpotLightProjectionExtension
{
	mat4 projection;					// projection transform
	textureHandle projectionTexture;	// projection texture
};

struct SpotLightShadowExtension
{
	mat4 projection;
	float shadowIntensity;				// intensity of shadows
	uint shadowSlice;
	textureHandle shadowMap;			// shadow map
};


struct PointLight
{
	vec4 position;				// view space position of light, w is range

	vec3 color;					// light color
	uint flags;					// feature flags (shadows, projection texture, etc)
};

struct PointLightShadowExtension
{
	float shadowIntensity;		// intensity of shadows
	uint shadowMap;				// shadow map
};

// match these in lightcontext.cc
const uint USE_SHADOW_BITFLAG = 1;
const uint USE_PROJECTION_TEX_BITFLAG = 2;

#define FlagSet(x, flags) ((x & flags) == flags)

samplerstate PointLightTextureSampler
{
	Filter = MinMagLinearMipPoint;
};

samplerstate SpotlightTextureSampler
{
	//Samplers = { LightProjMap, LightProjCube };
	Filter = MinMagLinearMipPoint;
	AddressU = Border;
	AddressV = Border;
	BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f };
};

#define SPECULAR_SCALE 13
#define ROUGHNESS_TO_SPECPOWER(x) exp2(SPECULAR_SCALE * x + 1)


//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
GetInvertedOcclusionSpotLight(float receiverDepthInLightSpace,
	vec2 lightSpaceUv,
	uint Index,
	uint Texture)
{

	// offset and scale shadow lookup tex coordinates
	vec3 shadowUv = vec3(lightSpaceUv, Index);

	// calculate average of 4 closest pixels
	vec2 shadowSample = sample2DArray(Texture, SpotlightTextureSampler, shadowUv).rg;

	// get pixel size of shadow projection texture
	return ChebyshevUpperBound(shadowSample, receiverDepthInLightSpace, 0.000001f);
}


//---------------------------------------------------------------------------------------------------------------------------
/**
*/
float
GetInvertedOcclusionPointLight(
	float receiverDepthInLightSpace,
	vec3 lightSpaceUv,
	uint Texture)
{

	// offset and scale shadow lookup tex coordinates
	vec3 shadowUv = lightSpaceUv;

	// get pixel size of shadow projection texture
	vec2 shadowSample = sampleCube(Texture, PointLightTextureSampler, shadowUv).rg;

	// get pixel size of shadow projection texture
	return ChebyshevUpperBound(shadowSample, receiverDepthInLightSpace, 0.00000001f);
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculatePointLight(
	in PointLight light, 
	in PointLightShadowExtension ext, 
	in vec3 viewPos,
	in vec3 viewVec, 
	in vec3 normal, 
	in float depth, 
	in vec4 material, 
	in vec4 albedo)
{
	vec3 lightDir = (light.position.xyz - viewPos);
	vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;

	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);
    float att = (sf * sf) / max(d2, 0.0001);

	lightDir = lightDir * (1 / lightDirLen);

	vec3 H = normalize(lightDir.xyz + viewVec);
	float NL = saturate(dot(lightDir, normal));
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, lightDir.xyz)); 
	
	vec3 F0 = vec3(0.04);
	CalculateF0(albedo.rgb, material[MAT_METALLIC], F0);

	vec3 fresnel;
	vec3 brdf;
	CalculateBRDF(NH, NL, NV, HL, material[MAT_ROUGHNESS], F0, fresnel, brdf);

	//Fresnel term (F) denotes the specular contribution of any light that hits the surface
	//We set kS (specular) to F and because PBR requires the condition that our equation is
	//energy conserving, we can set kD (diffuse contribution) to 1 - kS directly
	//as kS represents the energy of light that gets reflected, the remeaining energy gets refracted, resulting in our diffuse term
	vec3 kD = vec3(1.0f) - fresnel;

	//Fully metallic surfaces won't refract any light
	kD *= 1.0f - material[MAT_METALLIC];

	vec3 radiance = light.color * att;
	vec3 irradiance = (kD * albedo.rgb / PI + brdf) * radiance * saturate(NL);

	float shadowFactor = 1.0f;
	if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		shadowFactor = GetInvertedOcclusionPointLight(depth, projDir, ext.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ext.shadowIntensity));
	}

	return irradiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateSpotLight(
	in SpotLight light, 
	in SpotLightProjectionExtension projExt, 
	in SpotLightShadowExtension shadowExt, 
	in vec3 worldPos,
	in vec3 viewPos,
	in vec3 viewVec, 
	in vec3 normal, 
	in float depth, 
	in vec4 material, 
	in vec4 albedo)
{
	vec3 lightDir = (light.position.xyz - viewPos);
	
	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);

    float att = (sf * sf) / max(d2, 0.0001);

	lightDir = lightDir * (1 / lightDirLen);

	float theta = dot(light.forward.xyz, lightDir);
	float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

	vec4 lightModColor = intensity.xxxx * att;
	float shadowFactor = 1.0f;

	// if we have both projection and shadow extensions, transform only the projected position for one of them
	if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG) && (FlagSet(light.flags, USE_SHADOW_BITFLAG)))
	{
		vec4 projLightPos = projExt.projection * vec4(worldPos, 1.0f);
		vec2 lightSpaceUv = vec2(((projLightPos.xy / projLightPos.ww) * vec2(0.5f, 0.5f)) + 0.5f);
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);

		vec2 shadowLookup = (projLightPos.xy / projLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = projLightPos.z / projLightPos.w;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, shadowExt.shadowSlice, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}
	else if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG))
	{
		vec4 projLightPos = projExt.projection * vec4(worldPos, 1.0f);
		vec2 lightSpaceUv = vec2(((projLightPos.xy / projLightPos.ww) * vec2(0.5f, 0.5f)) + 0.5f);
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);
	}		
	else if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		// shadows
		vec4 shadowProjLightPos = shadowExt.projection * vec4(worldPos, 1.0f);
		vec2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = shadowProjLightPos.z / shadowProjLightPos.w;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, light.shadowExtension, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}

	vec3 H = normalize(lightDir.xyz + viewVec);
	float NL = saturate(dot(lightDir, normal));
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, lightDir.xyz)); 
	
	vec3 F0 = vec3(0.04);
	CalculateF0(albedo.rgb, material[MAT_METALLIC], F0);

	vec3 fresnel;
	vec3 brdf;
	CalculateBRDF(NH, NL, NV, HL, material[MAT_ROUGHNESS], F0, fresnel, brdf);

	//Fresnel term (F) denotes the specular contribution of any light that hits the surface
	//We set kS (specular) to F and because PBR requires the condition that our equation is
	//energy conserving, we can set kD (diffuse contribution) to 1 - kS directly
	//as kS represents the energy of light that gets reflected, the remeaining energy gets refracted, resulting in our diffuse term
	vec3 kD = vec3(1.0f) - fresnel;

	//Fully metallic surfaces won't refract any light
	kD *= 1.0f - material[MAT_METALLIC];

	vec3 radiance = light.color;
	vec3 irradiance = (kD * albedo.rgb / PI + brdf) * radiance * saturate(NL);

	return irradiance * shadowFactor * lightModColor.rgb;
}