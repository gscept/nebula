//------------------------------------------------------------------------------
//  lights_cluster_cull.fxh
//  (C) 2019 Gustav Sterbrant
//------------------------------------------------------------------------------

#include "lib/std.fxh"
#include "lib/util.fxh"
#include "lib/shared.fxh"
#include "lib/clustering.fxh"
#include "lib/lights_clustered.fxh"
#include "lib/CSM.fxh"
#include "lib/Preetham.fxh"

// increase if we need more lights in close proximity, for now, 128 is more than enough
#define MAX_LIGHTS_PER_CLUSTER 128

group(BATCH_GROUP) varblock LightConstants
{
	textureHandle SSAOBuffer;
};

group(BATCH_GROUP) varblock SpotLightList
{
	SpotLight SpotLights[1024];
};

group(BATCH_GROUP) varblock PointLightList
{
	PointLight PointLights[1024];
};

// this is used to keep track of how many lights we have active
group(BATCH_GROUP) varblock LightCullUniforms
{
	uint NumPointLights;
	uint NumSpotLights;
	uint NumClusters;
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
struct LightTileList
{
	uint lightIndex[MAX_LIGHTS_PER_CLUSTER];
};

group(BATCH_GROUP) varbuffer PointLightIndexLists
{
	uint PointLightIndexList[];
};

group(BATCH_GROUP) varbuffer PointLightCountLists
{
	uint PointLightCountList[];
};

group(BATCH_GROUP) varbuffer SpotLightIndexLists
{
	uint SpotLightIndexList[];
};

group(BATCH_GROUP) varbuffer SpotLightCountLists
{
	uint SpotLightCountList[];
};

write r11g11b10f image2D Lighting;

//------------------------------------------------------------------------------
/**
*/
bool 
TestAABBSphere(ClusterAABB aabb, vec3 pos, float radius)
{
	float sqDist = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float v = (pos)[i];

		if (v < aabb.minPoint[i]) sqDist += pow(aabb.minPoint[i] - v, 2);
		if (v > aabb.maxPoint[i]) sqDist += pow(v - aabb.maxPoint[i], 2);
	}
	return sqDist <= radius * radius;
}

//------------------------------------------------------------------------------
/**
	Treat AABB as a sphere.

	https://bartwronski.com/2017/04/13/cull-that-cone/
*/
bool
TestAABBCone(ClusterAABB aabb, vec3 pos, vec3 forward, float radius, vec2 sinCosAngles)
{
	float3 aabbExtents = (aabb.maxPoint.xyz - aabb.minPoint.xyz) * 0.5f;
	float3 aabbCenter = aabb.minPoint.xyz + aabbExtents;
	float aabbRadius = aabb.maxPoint.w;

	float3 v = aabbCenter - pos;
	const float vlensq = dot(v, v);
	const float v1len = dot(v, -forward); // weird, but forward must be pointing in the wrong direction...
	const float distanceClosestPoint = sinCosAngles.y * sqrt(vlensq - v1len * v1len) - v1len * sinCosAngles.x; 

	const bool angleCull	= distanceClosestPoint > aabbRadius;
	const bool frontCull	= v1len > aabbRadius + radius;
	const bool backCull		= v1len < -aabbRadius;
	return !(angleCull || backCull || frontCull);
}

write rgba16f image2D DebugOutput;

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 64
shader 
void csCull()
{
	uint index1D = gl_GlobalInvocationID.x;

	if (index1D > NumClusters)
		return;

	ClusterAABB aabb = AABBs[index1D];

	uint flags = 0;

	// update pointlights
	uint numLights = 0;
	for (uint i = 0; i < NumPointLights; i++)
	{
		const PointLight light = PointLights[i];
		if (TestAABBSphere(aabb, light.position.xyz, light.position.w))
		{
			PointLightIndexList[index1D * MAX_LIGHTS_PER_CLUSTER + numLights] = i;
			numLights++;
		}
	}
	PointLightCountList[index1D] = numLights;

	// update feature flags if we have any lights
	if (numLights > 0)
		flags |= CLUSTER_POINTLIGHT_BIT;

	// update spotlights
	numLights = 0;
	for (uint i = 0; i < NumSpotLights; i++)
	{
		const SpotLight light = SpotLights[i];

		// first do fast discard sphere test
		if (TestAABBSphere(aabb, light.position.xyz, light.position.w))
		{
			// then do more refined cone test, if previous test passed
			if (TestAABBCone(aabb, light.position.xyz, light.forward.xyz, light.position.w, light.angleSinCos))
			{
				SpotLightIndexList[index1D * MAX_LIGHTS_PER_CLUSTER + numLights] = i;
				numLights++;
			}
		}		
	}
	SpotLightCountList[index1D] = numLights;

	// update feature flags if we have any lights
	if (numLights > 0)
		flags |= CLUSTER_SPOTLIGHT_BIT;

	atomicOr(AABBs[index1D].featureFlags, flags);
}

//------------------------------------------------------------------------------
/**
*/
[localsizex] = 64
shader
void csLightDebug()
{
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;

	// convert screen coord to view-space position
	vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth);

	uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
	uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

	uint flag = AABBs[idx].featureFlags; // add 0 so we can read the value
	vec4 color = vec4(0, 0, 0, 0);
	if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
	{
		uint count = PointLightCountList[idx];
		color.r = count / float(NumPointLights);
	}
	if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
	{
		uint count = SpotLightCountList[idx];
		color.g = count / float(NumSpotLights);
	}
	
	imageStore(DebugOutput, int2(coord), color);
}

//------------------------------------------------------------------------------
/**
*/
vec3
GlobalLight(vec4 worldPos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo)
{
	float NL = saturate(dot(GlobalLightDirWorldspace.xyz, normal));
	if (NL <= 0) { return vec3(0,0,0); }

	float shadowFactor = 1.0f;
	vec4 debug = vec4(1, 1, 1, 1);
	if (FlagSet(GlobalLightFlags, USE_SHADOW_BITFLAG))
	{

		vec4 shadowPos = CSMShadowMatrix * worldPos; // csm contains inversed view + csm transform
		shadowFactor = CSMPS(shadowPos,
			GlobalLightShadowBuffer,
			debug);
		//debug = saturate(worldPos);
		shadowFactor = lerp(1.0f, shadowFactor, GlobalLightShadowIntensity);
	}
	else
	{
		debug = vec4(0, 0, 0, 0);
	}

	vec3 diff = GlobalAmbientLightColor.xyz;
	diff += GlobalLightColor.xyz * saturate(NL);
	//diff += GlobalBackLightColor.xyz * saturate(-NL + GlobalBackLightOffset);

	float specPower = ROUGHNESS_TO_SPECPOWER(material.a);

	vec3 H = normalize(GlobalLightDirWorldspace.xyz + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, GlobalLightDirWorldspace.xyz)); 
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, material.rgb, spec);

	vec3 final = (albedo.rgb + spec) * diff;
	return final * shadowFactor;
}

//------------------------------------------------------------------------------
/**
*/
vec3 
LocalLights(uint idx, vec4 viewPos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo)
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
			light += CalculatePointLight(
				li,
				ext,
				viewPos.xyz,
				viewVec,
				normal,
				depth,
				material,
				albedo
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
			light += CalculateSpotLight(
				li,
				projExt,
				shadowExt,
				viewPos.xyz,
				viewVec,
				normal,
				depth,
				material,
				albedo
			);
		}
	}
	return light;
}

//------------------------------------------------------------------------------
/**
	Calculate pixel light contribution
*/
[localsizex] = 64
shader
void csLighting()
{
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	vec3 normal = fetch2D(NormalBuffer, PosteffectSampler, coord, 0).rgb;
	float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;
	vec4 material = fetch2D(SpecularBuffer, PosteffectSampler, coord, 0).rgba;
	//material.a = 1.0f;
	float ssao = 1.0f - fetch2D(SSAOBuffer, PosteffectSampler, coord, 0).r;
	float ssaoSq = ssao * ssao;
	//material = vec4(1, 1, 1, 1.0f); 
	vec4 albedo = fetch2D(AlbedoBuffer, PosteffectSampler, coord, 0).rgba;

	// convert screen coord to view-space position
	vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth);
	vec4 worldPos = ViewToWorld(viewPos);
	vec3 worldViewVec = normalize(EyePos.xyz - worldPos.xyz);
	vec3 viewVec = -normalize(viewPos.xyz);
	vec3 viewNormal = (View * vec4(normal, 0)).xyz;

	uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias); 
	uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

	vec3 light = vec3(0,0,0); 

	// render lights where we have geometry
	if (normal.z != -1.0f)
	{
		// render global light
		light += GlobalLight(worldPos, worldViewVec, normal, depth, material, albedo);

		// render local lights
		light += LocalLights(idx, viewPos, viewVec, viewNormal, depth, material, albedo) * ssao;

		// reflections and irradiance
		vec3 reflectVec = reflect(-worldViewVec, normal.xyz);
		float x = dot(-viewNormal, worldViewVec);
		vec3 rim = FresnelSchlickGloss(material.rgb, x, material.a);
		vec3 reflection = sampleCubeLod(EnvironmentMap, CubeSampler, reflectVec, (1.0f - material.a) * NumEnvMips).rgb * rim * ssao;
		vec3 irradiance = sampleCubeLod(IrradianceMap, CubeSampler, normal.xyz, 0).rgb * ssao;
		float cavity = 1.0f; // todo: replace with material cavity
		light += (irradiance * albedo.rgb + reflection) * cavity;

		// sky light
		light += Preetham(normal, GlobalLightDirWorldspace.xyz, A, B, C, D, E, Z) * albedo.rgb * ssao;
	}	
	else // sky pixels
	{
		light += Preetham(normalize(worldPos.xyz), GlobalLightDirWorldspace.xyz, A, B, C, D, E, Z)* GlobalLightColor.xyz;
	}

	// write final output
	imageStore(Lighting, coord, light.xyzx);
}

//------------------------------------------------------------------------------
/**
*/
program CullLights [ string Mask = "Cull"; ]
{
	ComputeShader = csCull();
};

//------------------------------------------------------------------------------
/**
*/
program ClusterDebug [ string Mask = "ClusterDebug"; ]
{
	ComputeShader = csLightDebug();
};

//------------------------------------------------------------------------------
/**
*/
program LightingShade [ string Mask = "Lighting"; ]
{
	ComputeShader = csLighting();
};