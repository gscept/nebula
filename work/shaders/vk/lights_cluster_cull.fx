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

#define MAX_LIGHTS_PER_CLUSTER 32

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
};

// contains amount of lights, and the index of the light (pointing to the indices in PointLightList and SpotLightList), to output
struct LightTileList
{
	uint numLights;
	uint lightIndex[MAX_LIGHTS_PER_CLUSTER];
};

group(BATCH_GROUP) varbuffer PointLightIndexLists
{
	LightTileList PointLightIndexList[];
};

group(BATCH_GROUP) varbuffer SpotLightIndexLists
{
	LightTileList SpotLightIndexList[];
};

write r11g11b10f image2D Lighting;

//------------------------------------------------------------------------------
/**
*/
bool 
TestAABBPointLight(ClusterAABB aabb, PointLight light)
{
	float sqDist = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		float v = (light.position.xyz)[i];

		if (v < aabb.minPoint[i]) sqDist += pow(aabb.minPoint[i] - v, 2);
		if (v > aabb.maxPoint[i]) sqDist += pow(v - aabb.maxPoint[i], 2);
	}
	return sqDist <= light.position.w;
}

//------------------------------------------------------------------------------
/**
	Treat AABB as a sphere.

	https://bartwronski.com/2017/04/13/cull-that-cone/
*/
bool
TestAABBCone(ClusterAABB aabb, SpotLight light)
{
	float3 aabbExtents = (aabb.maxPoint.xyz - aabb.minPoint.xyz) * 0.5f;
	float3 aabbCenter = aabb.minPoint.xyz + aabbExtents;
	float aabbRadius = dot(aabbExtents, aabbExtents);

	float3 v = aabbCenter - light.position.xyz;
	const float vlensq = dot(v, v);
	const float v1len = dot(v, light.forward.xyz);
	const float distanceClosestPoint = cos(light.angle.y) * sqrt(vlensq - v1len * v1len) - v1len * sin(light.angle.y);

	const bool angleCull = distanceClosestPoint > aabbRadius;
	const bool frontCull = v1len > aabbRadius + light.position.w * light.position.w;
	const bool backCull = v1len < -aabbRadius;
	return !(angleCull || frontCull || backCull);
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
	ClusterAABB aabb = AABBs[index1D];

	uint flags = 0;

	// update pointlights
	LightTileList pointCell;
	pointCell.numLights = 0;
	for (uint i = 0; i < NumPointLights && pointCell.numLights < MAX_LIGHTS_PER_CLUSTER; i++)
	{
		const PointLight light = PointLights[i];
		if (TestAABBPointLight(aabb, light))
		{
			pointCell.lightIndex[pointCell.numLights] = i;
			pointCell.numLights++;
		}
	}
	PointLightIndexList[index1D] = pointCell;

	// update feature flags if we have any lights
	if (pointCell.numLights > 0)
		flags |= CLUSTER_POINTLIGHT_BIT;

	// update spotlights
	LightTileList spotCell;
	spotCell.numLights = 0;

	for (uint i = 0; i < NumSpotLights && spotCell.numLights < MAX_LIGHTS_PER_CLUSTER; i++)
	{
		const SpotLight light = SpotLights[i];
		if (TestAABBCone(aabb, light))
		//if (true)
		{
			spotCell.lightIndex[spotCell.numLights] = i;
			spotCell.numLights++;
		}
	}
	SpotLightIndexList[index1D] = spotCell;

	// update feature flags if we have any lights
	if (spotCell.numLights > 0)
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

	uint flag = atomicAdd(AABBs[idx].featureFlags, 0); // add 0 so we can read the value
	vec4 color = vec4(0, 0, 0, 0);
	if (CHECK_FLAG(flag, CLUSTER_POINTLIGHT_BIT))
	{
		LightTileList cell = PointLightIndexList[idx];
		color.r = cell.numLights / 7.0f;
	}
	if (CHECK_FLAG(flag, CLUSTER_SPOTLIGHT_BIT))
	{
		LightTileList cell = SpotLightIndexList[idx];
		color.g = cell.numLights / 7.0f;
	}
	
	imageStore(DebugOutput, int2(coord), color);
}

//------------------------------------------------------------------------------
/**
*/
vec3
GlobalLight(vec4 worldPos, vec3 viewVec, vec3 normal, float depth, vec4 material, vec4 albedo)
{
	if (normal.z == -1.0f) { return albedo.rgb; }

	float NL = saturate(dot(GlobalLightDir.xyz, normal));
	if (NL <= 0) { return albedo.rgb; }

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

	vec3 H = normalize(GlobalLightDir.xyz + viewVec);
	float NH = saturate(dot(normal, H));
	float NV = saturate(dot(normal, viewVec));
	float HL = saturate(dot(H, GlobalLightDir.xyz));
	vec3 spec;
	BRDFLighting(NH, NL, NV, HL, specPower, material.rgb, spec);

	// add sky light
	vec3 skyLight = Preetham(normal, GlobalLightDir.xyz, A, B, C, D, E, Z) * GlobalLightColor.xyz;
	diff += skyLight;

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
		LightTileList cell = PointLightIndexList[idx];
		PointLightShadowExtension ext;
		for (int i = 0; i < cell.numLights; i++)
		{
			uint lidx = cell.lightIndex[i];
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
		LightTileList cell = SpotLightIndexList[idx];
		SpotLightShadowExtension shadowExt;
		SpotLightProjectionExtension projExt;
		for (int i = 0; i < cell.numLights; i++)
		{
			uint lidx = cell.lightIndex[i];
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
	Performs actual light shading
*/
[localsizex] = 64
shader
void csLighting()
{
	ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
	vec3 normal = fetch2D(NormalBuffer, PosteffectSampler, coord, 0).rgb;
	float depth = fetch2D(DepthBuffer, PosteffectSampler, coord, 0).r;
	vec4 material = fetch2D(SpecularBuffer, PosteffectSampler, coord, 0).rgba;
	vec4 albedo = fetch2D(AlbedoBuffer, PosteffectSampler, coord, 0).rgba;

	// convert screen coord to view-space position
	vec4 viewPos = PixelToView(coord * InvFramebufferDimensions, depth);
	vec4 worldPos = ViewToWorld(viewPos);
	vec3 viewVec = EyePos.xyz - worldPos.xyz;
	vec3 viewNormal = (View * vec4(normal, 0)).xyz;

	uint3 index3D = CalculateClusterIndex(coord / BlockSize, viewPos.z, InvZScale, InvZBias);
	uint idx = Pack3DTo1D(index3D, NumCells.x, NumCells.y);

	vec3 light = vec3(0,0,0);

	// render global light
	light += GlobalLight(worldPos, viewVec, normal, depth, material, albedo);

	// render local lights
	light += LocalLights(idx, viewPos, viewVec, viewNormal, depth, material, albedo);
	
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