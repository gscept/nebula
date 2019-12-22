//------------------------------------------------------------------------------
//  lights_clustered.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "lib/shadowbase.fxh"

struct SpotLight
{
	vec4 position;			// view space position of light, w is range
	vec4 forward;			// forward vector of light (spotlight and arealights)

	vec2 angleSinCos;		// angle cutoffs

	vec3 color;				// light color
	uint flags;				// feature flags (shadows, projection texture, etc)
};

struct SpotLightProjectionExtension
{
	mat4 projection;		// projection transform
	uint projectionTexture;	// projection texture
};

struct SpotLightShadowExtension
{
	mat4 projection;
	vec4 shadowOffsetScale;
	float shadowIntensity;	// intensity of shadows
	uint shadowMap;			// shadow map
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
	vec4 shadowOffsetAndScale,
	uint Texture)
{

	// offset and scale shadow lookup tex coordinates
	vec2 shadowUv = lightSpaceUv;
	shadowUv.xy *= shadowOffsetAndScale.zw;
	shadowUv.xy += shadowOffsetAndScale.xy;

	// calculate average of 4 closest pixels
	vec2 shadowSample = sample2D(Texture, SpotlightTextureSampler, shadowUv).rg;

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

	float att = saturate(1.0 - length(lightDir) * 1/light.position.w);
	if (att - 0.004 < 0) return vec3(0, 0, 0);
	//att *= att;
	lightDir = normalize(lightDir);

	float specPower = ROUGHNESS_TO_SPECPOWER(material.a);	// magic formulae to calculate specular power from color in the range [0..1]

	float NL = saturate(dot(lightDir, normal));
	vec3 diff = light.color * saturate(NL);

	vec3 H = normalize(lightDir + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, material.rgb, spec);
	vec3 final = (albedo.rgb + spec) * diff;

	// shadows
	float shadowFactor = 1.0f;
	if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		shadowFactor = GetInvertedOcclusionPointLight(depth, projDir, ext.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ext.shadowIntensity));
	}

	return final * shadowFactor * att;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateSpotLight(
	in SpotLight light, 
	in SpotLightProjectionExtension projExt, 
	in SpotLightShadowExtension shadowExt, 
	in vec3 viewPos,
	in vec3 viewVec, 
	in vec3 normal, 
	in float depth, 
	in vec4 material, 
	in vec4 albedo)
{
	vec3 lightDir = (light.position.xyz - viewPos);
	float att = saturate(1.0 - length(lightDir) * 1 / light.position.w);
	if (att - 0.004 < 0) return vec3(0, 0, 0);
	//att *= att;
	lightDir = normalize(lightDir);

	float theta = dot(light.forward.xyz, lightDir);
	float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

	vec4 projLightPos = vec4(0, 0, 0, 0);
	vec4 lightModColor = intensity.xxxx * att;
	if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG) || FlagSet(light.flags, USE_SHADOW_BITFLAG))
		projLightPos = projExt.projection * vec4(viewPos, 1.0f);
	
	if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG))
	{
		vec2 lightSpaceUv = vec2(((projLightPos.xy / projLightPos.ww) * vec2(0.5f, 0.5f)) + 0.5f);
		lightModColor = sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);
	}		

	float shadowFactor = 1.0f;
	if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		// shadows
		vec4 shadowProjLightPos = shadowExt.projection * vec4(viewPos, 1.0f);
		vec2 shadowLookup = (shadowProjLightPos.xy / shadowProjLightPos.ww) * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = projLightPos.z / projLightPos.w;
		shadowFactor = GetInvertedOcclusionSpotLight(depth, shadowLookup, shadowExt.shadowOffsetScale, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}

	float specPower = ROUGHNESS_TO_SPECPOWER(material.a);

	float NL = saturate(dot(lightDir, normal));
	vec3 diff = light.color.xyz * saturate(NL);

	vec3 H = normalize(lightDir + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, lightDir));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, material.rgb, spec);
	vec3 final = (albedo.rgb + spec) * diff;

	return final * shadowFactor * lightModColor.rgb;
}