//------------------------------------------------------------------------------
//  lights_clustered.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "shadowbase.fxh"
#include "pbr.fxh"
#include "CSM.fxh"

// increase if we need more lights in close proximity, for now, 128 is more than enough
const uint MAX_LIGHTS_PER_CLUSTER = 128;

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

#ifndef LIGHTS_CLUSTERED_GROUP
#define LIGHTS_CLUSTERED_GROUP BATCH_GROUP
#endif

#ifndef LIGHTS_CLUSTERED_VISIBILITY
#define LIGHTS_CLUSTERED_VISIBILITY "CS|PS"
#endif

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
group(LIGHTS_CLUSTERED_GROUP) rw_buffer LightIndexLists[string Visibility = LIGHTS_CLUSTERED_VISIBILITY;]
{
	uint PointLightCountList[NUM_CLUSTER_ENTRIES];
	uint PointLightIndexList[NUM_CLUSTER_ENTRIES * MAX_LIGHTS_PER_CLUSTER];
	uint SpotLightCountList[NUM_CLUSTER_ENTRIES];
	uint SpotLightIndexList[NUM_CLUSTER_ENTRIES * MAX_LIGHTS_PER_CLUSTER];
};

group(LIGHTS_CLUSTERED_GROUP) rw_buffer LightLists[string Visibility = LIGHTS_CLUSTERED_VISIBILITY;]
{
	SpotLight SpotLights[1024];
	SpotLightProjectionExtension SpotLightProjection[256];
	SpotLightShadowExtension SpotLightShadow[16];
	PointLight PointLights[1024];
};

// match these in lightcontext.cc
const uint USE_SHADOW_BITFLAG = 1;
const uint USE_PROJECTION_TEX_BITFLAG = 2;

#define FlagSet(x, flags) ((x & flags) == flags)

sampler_state PointLightTextureSampler
{
	Filter = MinMagLinearMipPoint;
};

sampler_state SpotlightTextureSampler
{
	//Samplers = { LightProjMap, LightProjCube };
	Filter = MinMagLinearMipPoint;
	AddressU = Border;
	AddressV = Border;
	BorderColor = Transparent;
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
	Calculate point light for ambient, specular and shadows
*/
vec3
CalculatePointLight(
	in PointLight light, 
	in PointLightShadowExtension ext, 
	in vec3 viewPos,
	in vec3 viewVec, 
	in vec3 viewSpaceNormal, 
	in float depth, 
	in vec4 material, 
	in vec3 diffuseColor,
	in vec3 F0)
{
	vec3 lightDir = (light.position.xyz - viewPos);
	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);
    float att = (sf * sf) / max(d2, 0.0001);

	float oneDivLightDirLen = 1.0f / lightDirLen;
	lightDir = lightDir * oneDivLightDirLen;

	vec3 H = normalize(lightDir.xyz + viewVec);
	float NL = saturate(dot(lightDir, viewSpaceNormal));
	float NH = saturate(dot(viewSpaceNormal, H));
	float NV = saturate(dot(viewSpaceNormal, viewVec));
	float LH = saturate(dot(H, lightDir.xyz)); 

	vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

	vec3 radiance = light.color * att;
	vec3 irradiance = (brdf * radiance) * saturate(NL);

	float shadowFactor = 1.0f;
	if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
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
	in vec3 viewPos,
	in vec3 viewVec, 
	in vec3 viewSpaceNormal, 
	in float depth, 
	in vec4 material, 
	in vec3 diffuseColor,
	in vec3 F0)
{
	vec3 lightDir = (light.position.xyz - viewPos);

	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
    float factor = d2 / (light.position.w * light.position.w);
    float sf = saturate(1.0 - factor * factor);

    float att = (sf * sf) / max(d2, 0.0001);

	float oneDivLightDirLen = 1.0f / lightDirLen;
	lightDir = lightDir * oneDivLightDirLen;

	float theta = dot(light.forward.xyz, lightDir);
	float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

	vec4 lightModColor = intensity.xxxx * att;
	float shadowFactor = 1.0f;

	// if we have both projection and shadow extensions, transform only the projected position for one of them
	if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG) && (FlagSet(light.flags, USE_SHADOW_BITFLAG)))
	{
		vec4 projLightPos = projExt.projection * vec4(viewPos, 1.0f);
		projLightPos.xyz /= projLightPos.www;
		vec2 lightSpaceUv = projLightPos.xy * vec2(0.5f, 0.5f) + 0.5f;
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);

		vec2 shadowLookup = projLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = projLightPos.z;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, shadowExt.shadowSlice, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}
	else if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG))
	{
		vec4 projLightPos = projExt.projection * vec4(viewPos, 1.0f);
		projLightPos.xy /= projLightPos.ww;
		vec2 lightSpaceUv = projLightPos.xy * vec2(0.5f, 0.5f) + 0.5f;
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);
	}		
	else if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		// shadows
		vec4 shadowProjLightPos = shadowExt.projection * vec4(viewPos, 1.0f);
		shadowProjLightPos.xyz /= shadowProjLightPos.www;
		vec2 shadowLookup = shadowProjLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = shadowProjLightPos.z;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, light.shadowExtension, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}

	vec3 H = normalize(lightDir.xyz + viewVec);
	float NL = saturate(dot(lightDir, viewSpaceNormal));
	float NH = saturate(dot(viewSpaceNormal, H));
	float NV = saturate(dot(viewSpaceNormal, viewVec));
	float LH = saturate(dot(H, lightDir.xyz)); 
	
	vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

	vec3 radiance = light.color;
	vec3 irradiance = (brdf * radiance) * saturate(NL);

	return irradiance * shadowFactor * lightModColor.rgb;
}

//------------------------------------------------------------------------------
/**
	@param diffuseColor		Material's diffuse reflectance color
	@param material			Material parameters (metallic, roughness, cavity)
	@param F0				Fresnel reflectance at 0 degree incidence angle
	@param viewVec			Unit vector from cameras worldspace position to fragments world space position.
	@param viewSpacePos		Fragments position in viewspace; used for shadowing.
*/
vec3
CalculateGlobalLight(vec3 diffuseColor, vec4 material, vec3 F0, vec3 viewVec, vec3 worldSpaceNormal, vec4 viewSpacePos)
{
	float NL = saturate(dot(GlobalLightDirWorldspace.xyz, worldSpaceNormal));
	if (NL <= 0) { return vec3(0); }

	float shadowFactor = 1.0f;
	if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
	{
		vec4 shadowPos = CSMShadowMatrix * viewSpacePos; // csm contains inversed view + csm transform
		shadowFactor = CSMPS(shadowPos,	GlobalLightShadowBuffer);
		shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
	}

	vec3 H = normalize(GlobalLightDirWorldspace.xyz + viewVec);
	float NV = saturate(dot(worldSpaceNormal, viewVec));
	float NH = saturate(dot(worldSpaceNormal, H));
	float LH = saturate(dot(H, GlobalLightDirWorldspace.xyz));

	vec3 brdf = EvaluateBRDF(diffuseColor, material, F0, H, NV, NL, NH, LH);

	vec3 radiance = GlobalLightColor.xyz;
	vec3 irradiance = (brdf * radiance) * saturate(NL) + GlobalAmbientLightColor.xyz;
	return irradiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
	@param clusterIndex		The 1D cluster/bucket index to evaluate
	@param diffuseColor		Material's diffuse reflectance color
	@param material			Material parameters (metallic, roughness, cavity)
	@param F0				Fresnel reflectance at 0 degree incidence angle
	@param viewPos			The fragments position in view space
	@param viewSpaceNormal	The fragments view space normal
	@param depth			The fragments depth (gl_FragCoord.z)
*/
vec3
LocalLights(uint clusterIndex, vec3 diffuseColor, vec4 material, vec3 F0, vec4 viewPos, vec3 viewSpaceNormal, float depth)
{
	vec3 light = vec3(0, 0, 0);
	uint flag = AABBs[clusterIndex].featureFlags;
	vec3 viewVec = -normalize(viewPos.xyz);
	if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
	{
		// shade point lights
		uint count = PointLightCountList[clusterIndex];
		PointLightShadowExtension ext;
		for (int i = 0; i < count; i++)
		{
			uint lidx = PointLightIndexList[clusterIndex * MAX_LIGHTS_PER_CLUSTER + i];
			PointLight li = PointLights[lidx];
			light += CalculatePointLight(
				li,
				ext,
				viewPos.xyz,
				viewVec,
				viewSpaceNormal,
				depth,
				material,
				diffuseColor,
				F0
			);
		}
	}
	if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
	{
		uint count = SpotLightCountList[clusterIndex];
		SpotLightShadowExtension shadowExt;
		SpotLightProjectionExtension projExt;
		for (int i = 0; i < count; i++)
		{
			uint lidx = SpotLightIndexList[clusterIndex * MAX_LIGHTS_PER_CLUSTER + i];
			SpotLight li = SpotLights[lidx];

			// if we have extensions, load them from their respective buffers
			if (li.shadowExtension != -1)
				shadowExt = SpotLightShadow[li.shadowExtension];
			if (li.projectionExtension != -1)
				projExt = SpotLightProjection[li.projectionExtension];

			light += CalculateSpotLight(
				li,
				projExt,
				shadowExt,
				viewPos.xyz,
				viewVec,
				viewSpaceNormal,
				depth,
				material,
				diffuseColor,
				F0
			);
		}
	}
	return light;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculatePointLightAmbientTransmission(
	in PointLight light,
	in PointLightShadowExtension ext,
	in vec3 viewPos,
	in vec3 viewVec,
	in vec3 normal,
	in float depth,
	in vec4 material,
	in vec4 albedo,
	in float transmission)
{
	vec3 lightDir = (light.position.xyz - viewPos);
	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
	float factor = d2 / (light.position.w * light.position.w);
	float sf = saturate(1.0 - factor * factor);
	float att = (sf * sf) / max(d2, 0.0001);
	lightDir = lightDir / lightDirLen;

	float NL = saturate(dot(lightDir, normal));
	float TNL = saturate(dot(-lightDir, normal)) * transmission;
	vec3 radiance = light.color * att * saturate(NL + TNL) * albedo.rgb;

	float shadowFactor = 1.0f;
	if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		vec3 projDir = (InvView * vec4(-lightDir, 0)).xyz;
		shadowFactor = GetInvertedOcclusionPointLight(depth, projDir, ext.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), ext.shadowIntensity));
	}

	return radiance * shadowFactor;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateSpotLightAmbientTransmission(
	in SpotLight light,
	in SpotLightProjectionExtension projExt,
	in SpotLightShadowExtension shadowExt,
	in vec3 viewPos,
	in vec3 viewVec,
	in vec3 normal,
	in float depth,
	in vec4 material,
	in vec4 albedo,
	in float transmission)
{
	vec3 lightDir = (light.position.xyz - viewPos);

	float lightDirLen = length(lightDir);

	float d2 = lightDirLen * lightDirLen;
	float factor = d2 / (light.position.w * light.position.w);
	float sf = saturate(1.0 - factor * factor);

	float att = (sf * sf) / max(d2, 0.0001);
	lightDir = lightDir / lightDirLen;

	float theta = dot(light.forward.xyz, lightDir);
	float intensity = saturate((theta - light.angleSinCos.y) * light.forward.w);

	vec4 lightModColor = intensity.xxxx * att;
	float shadowFactor = 1.0f;

	// if we have both projection and shadow extensions, transform only the projected position for one of them
	if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG) && (FlagSet(light.flags, USE_SHADOW_BITFLAG)))
	{
		vec4 projLightPos = projExt.projection * vec4(viewPos, 1.0f);
		projLightPos.xyz /= projLightPos.www;
		vec2 lightSpaceUv = projLightPos.xy * vec2(0.5f, 0.5f) + 0.5f;
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);

		vec2 shadowLookup = projLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = projLightPos.z;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, shadowExt.shadowSlice, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}
	else if (FlagSet(light.flags, USE_PROJECTION_TEX_BITFLAG))
	{
		vec4 projLightPos = projExt.projection * vec4(viewPos, 1.0f);
		projLightPos.xy /= projLightPos.ww;
		vec2 lightSpaceUv = projLightPos.xy * vec2(0.5f, 0.5f) + 0.5f;
		lightModColor *= sample2DLod(projExt.projectionTexture, SpotlightTextureSampler, lightSpaceUv, 0);
	}
	else if (FlagSet(light.flags, USE_SHADOW_BITFLAG))
	{
		// shadows
		vec4 shadowProjLightPos = shadowExt.projection * vec4(viewPos, 1.0f);
		shadowProjLightPos.xyz /= shadowProjLightPos.www;
		vec2 shadowLookup = shadowProjLightPos.xy * vec2(0.5f, -0.5f) + 0.5f;
		shadowLookup.y = 1 - shadowLookup.y;
		float receiverDepth = shadowProjLightPos.z;
		shadowFactor = GetInvertedOcclusionSpotLight(receiverDepth, shadowLookup, light.shadowExtension, shadowExt.shadowMap);
		shadowFactor = saturate(lerp(1.0f, saturate(shadowFactor), shadowExt.shadowIntensity));
	}

	float NL = saturate(dot(lightDir, normal));
	float TNL = saturate(dot(-lightDir, normal)) * transmission;
	vec3 radiance = light.color * saturate(NL + TNL) * albedo.rgb;

	return radiance * shadowFactor * lightModColor.rgb;
}

//------------------------------------------------------------------------------
/**
*/
vec3
CalculateGlobalLightAmbientTransmission(vec4 viewPos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo, in float transmission)
{
	float NL = saturate(dot(GlobalLightDirWorldspace.xyz, normal));
	float TNL = saturate(dot(-GlobalLightDirWorldspace.xyz, normal)) * transmission;

	if ((NL + TNL) <= 0) { return vec3(0); }

	float shadowFactor = 1.0f;
	if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
	{
		vec4 shadowPos = CSMShadowMatrix * viewPos; // csm contains inversed view + csm transform
		shadowFactor = CSMPS(shadowPos, GlobalLightShadowBuffer);
		shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
	}

	vec3 radiance = GlobalLightColor.xyz * saturate(NL + TNL);

	return radiance * shadowFactor * albedo.rgb;
}

//------------------------------------------------------------------------------
/**
*/
vec3
LocalLightsAmbientTransmission(
	uint idx,
	vec4 viewPos,
	vec3 viewVec,
	vec3 normal,
	float depth,
	vec4 material,
	vec4 albedo,
	float transmission)
{
	vec3 light = vec3(0, 0, 0);
	uint flag = AABBs[idx].featureFlags;
	if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
	{
		// shade point lights
		uint count = PointLightCountList[idx];
		PointLightShadowExtension ext;
		for (int i = 0; i < count; i++)
		{
			uint lidx = PointLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
			PointLight li = PointLights[lidx];
			light += CalculatePointLightAmbientTransmission(
				li,
				ext,
				viewPos.xyz,
				viewVec,
				normal,
				depth,
				material,
				albedo,
				transmission
			);
		}
	}
	if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
	{
		uint count = SpotLightCountList[idx];
		SpotLightShadowExtension shadowExt;
		SpotLightProjectionExtension projExt;
		for (int i = 0; i < count; i++)
		{
			uint lidx = SpotLightIndexList[idx * MAX_LIGHTS_PER_CLUSTER + i];
			SpotLight li = SpotLights[lidx];

			// if we have extensions, load them from their respective buffers
			if (li.shadowExtension != -1)
				shadowExt = SpotLightShadow[li.shadowExtension];
			if (li.projectionExtension != -1)
				projExt = SpotLightProjection[li.projectionExtension];

			light += CalculateSpotLightAmbientTransmission(
				li,
				projExt,
				shadowExt,
				viewPos.xyz,
				viewVec,
				normal,
				depth,
				material,
				albedo,
				transmission
			);
		}
	}
	return light;
}